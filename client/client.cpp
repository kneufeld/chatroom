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
#include <cstring>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;

#include <msgpack.hpp>

#include "common.hpp"

typedef std::array<char, 1024> buffer_t;
using std::cout;
using std::cerr;
using std::endl;

typedef std::deque<chat_message> chat_message_queue;

typedef boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type> iterator;
std::pair<iterator, bool>match( iterator begin, iterator end )
{
    int size = end - begin;
    // XXX FIXME
    msgpack::unpacker m_unpacker;
    m_unpacker.reserve_buffer( size );
    std::copy( begin, end, m_unpacker.buffer() );
    m_unpacker.buffer_consumed( size );
    
    msgpack::unpacked result;
    bool got_one = m_unpacker.next( &result );
    return std::make_pair( end, got_one );
};


class chat_client
{
public:
    chat_client( boost::asio::io_service& io_service,
                 tcp::resolver::iterator endpoint_iterator )
        : io_service_( io_service ),
          socket_( io_service ),
          m_unpacker(),
          input_( io_service, ::dup( STDIN_FILENO ) ),
          output_( io_service, ::dup( STDOUT_FILENO ) ),
          input_buffer_( 256 )
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
            std::copy( sbuf.data(), sbuf.data() + sbuf.size(), write_msg_.data() );
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
                close();
                boost::asio::detail::throw_error( ec, "on read" );
            }
            
            // feed data to unpacker
            m_unpacker.reserve_buffer( length );
            std::copy( read_msg_.data(), read_msg_.data() + length, m_unpacker.buffer() );
            m_unpacker.buffer_consumed( length );
            
            // maybe-get object out of it
            msgpack::unpacked result;
            
            if( m_unpacker.next( &result ) )
            {
                result.get().convert( &msg );
                cout << endl << msg.nickname << ": " << msg.message << endl;
            }
            
            do_read_header();
        } );
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
                close();
                boost::asio::detail::throw_error( ec, "on write" );
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
    
    posix::stream_descriptor input_;
    posix::stream_descriptor output_;
    boost::asio::streambuf input_buffer_;
    msgpack::unpacker m_unpacker;
    
};
