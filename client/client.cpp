//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

#include <msgpack.hpp>

typedef std::array<char, 1024> buffer_t;
using std::cout;
using std::cerr;
using std::endl;

class chat_message
{
public:
    chat_message() {}

    std::string nickname;
    std::string msg;

    MSGPACK_DEFINE( nickname, msg );
};

typedef std::deque<chat_message> chat_message_queue;

typedef boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type> iterator;
std::pair<iterator, bool>match(iterator begin, iterator end)
{
    int size = end - begin;
    // XXX FIXME
    msgpack::unpacker unpacker_;
    unpacker_.reserve_buffer(size);
    std::copy(begin, end, unpacker_.buffer());
    unpacker_.buffer_consumed(size);

    msgpack::unpacked result;
    bool got_one = unpacker_.next(&result);
    return std::make_pair(end, got_one);
};


class chat_client
{
    msgpack::unpacker unpacker_;

public:
    chat_client( boost::asio::io_service& io_service,
                 tcp::resolver::iterator endpoint_iterator )
        : io_service_( io_service ),
          socket_( io_service ),
          unpacker_()
    {
        do_connect( endpoint_iterator );
    }

    void write( const chat_message& msg )
    {
        io_service_.post(
            [this, msg]()
        {
            msgpack::sbuffer sbuf;
            msgpack::pack( sbuf, msg );
            //memcpy( write_msg_.data(), sbuf.data(), sbuf.size() );
            std::copy(sbuf.data(), sbuf.data() + sbuf.size(), write_msg_.data() );
            do_write( sbuf.size() );
        } );
    }

    void close()
    {
        io_service_.post( [this]() { socket_.close(); } );
    }

private:
    void do_connect( tcp::resolver::iterator endpoint_iterator )
    {
        boost::asio::async_connect( socket_, endpoint_iterator,
                                    [this]( boost::system::error_code ec, tcp::resolver::iterator )
        {
            if( !ec )
            {
                do_read_header();
            }
        } );
    }

    void do_read_header()
    {
        do_read_body();
    }


    void do_read_body()
    {
        socket_.async_read_some(
            boost::asio::buffer( read_msg_.data(), 1024 ),
            [this]( boost::system::error_code ec, std::size_t length )
            {
                if( ec )
                {
                    socket_.close();
                    return;
                }

                // feed data to unpacker
                unpacker_.reserve_buffer(length);
                std::copy(read_msg_.data(), read_msg_.data() + length,
                          unpacker_.buffer());
                unpacker_.buffer_consumed(length);

                // maybe-get object out of it
                msgpack::unpacked result;
                if (unpacker_.next(&result)) {
                    result.get().convert(&msg);
                    cout << endl << msg.nickname << ": " << msg.msg << endl;
                }
                do_read_header();
            });
    }

    void do_write( unsigned msg_length )
    {
//        std::array<char, 1> b = { ( char )msg_length };
//        boost::asio::write( socket_, boost::asio::buffer( b, 1 ) );

        boost::asio::async_write( socket_,
                                  boost::asio::buffer( write_msg_.data(), msg_length ),
                                  [this]( boost::system::error_code ec, std::size_t length )
        {
            if( ec )
            {
                std::cerr << "socket error";
                socket_.close();
                return;
            }

            //std::cout << "sent " << length << endl;
        } );
    }

private:
    boost::asio::io_service& io_service_;
    tcp::socket socket_;

    buffer_t read_msg_;
    buffer_t write_msg_;
    chat_message msg;
};

int main( int argc, char* argv[] )
{
    try
    {
        if( argc != 4 )
        {
            std::cerr << "Usage: chat_client <nickname> <host> <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;

        tcp::resolver resolver( io_service );
        auto endpoint_iterator = resolver.resolve( { argv[2], argv[3] } );
        chat_client c( io_service, endpoint_iterator );

        std::thread t( [&io_service]() { io_service.run(); } );

        char line[256];

        cout << argv[1] << ": " << std::flush;

        while( std::cin.getline( line, 255 ) )
        {
            chat_message msg;
            msg.nickname = argv[1];

            std::string l = line;
            l.erase( std::remove( l.begin(), l.end(), '\n' ), l.end() );
            msg.msg = l;

            c.write( msg );

            cout << argv[1] << ": " << std::flush;
        }

        c.close();
        t.join();
    }
    catch( std::exception& e )
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
