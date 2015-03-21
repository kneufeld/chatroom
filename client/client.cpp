#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>

#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "client.hpp"

using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;


posix_chat_client::posix_chat_client( boost::asio::io_service& io_service,
                                      tcp::resolver::iterator endpoint_iterator,
                                      std::string nickname )
    : socket_( io_service ),
      input_( io_service, ::dup( STDIN_FILENO ) ),
      output_( io_service, ::dup( STDOUT_FILENO ) ),
      input_buffer_( max_msg_length )
{
    m_nickname = nickname;
    
    // attempt to connect to server, call handle_connect when we do
    auto handler = boost::bind( &posix_chat_client::handle_connect, this, boost::asio::placeholders::error );
    boost::asio::async_connect( socket_, endpoint_iterator, handler );
}

void posix_chat_client::handle_connect( const boost::system::error_code& error )
{
    if( error )
    {
        std::cerr << "could not connect to " << socket_.remote_endpoint();
        return;
    }
    
    read_socket();
    read_input();
}

void posix_chat_client::read_socket()
{
    // read from socket
    auto buffers = boost::asio::buffer( read_msg_, 1024 );
    auto handler = boost::bind( &posix_chat_client::cb_read_socket, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred );
    socket_.async_read_some( buffers, handler );
}

void posix_chat_client::read_input()
{
    // read from console until newline
    auto handler = boost::bind( &posix_chat_client::cb_read_input, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred );
    boost::asio::async_read_until( input_, input_buffer_, '\n', handler );
}

void posix_chat_client::cb_read_socket( const boost::system::error_code& error, std::size_t bytes_recv )
{
    if( error )
    {
        std::cerr << "socket error: " << error.message() << std::endl;
        close();
        return;
    }
    
    if( feed_to_unpacker( read_msg_, bytes_recv ) )
    {
        static std::stringstream ss;
        ss.str( "" );
        ss << m_msg.nickname << ": " << m_msg.message << std::endl;
        
        static char eol[] = { '\n' };
        boost::array<boost::asio::const_buffer, 2> buffers =
        {
            boost::asio::buffer( ss.str(), ss.str().size() ),
            boost::asio::buffer( eol )
        };
        
        // write out the message we just received, terminated by a newline.
        auto handler = boost::bind( &posix_chat_client::cb_write_output, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred );
        boost::asio::async_write( output_, buffers, handler );
    }
    else
    {
        read_socket(); // read more bytes
    }
}

void posix_chat_client::cb_write_socket( const boost::system::error_code& error, std::size_t length )
{
    if( !error )
    {
        read_input();
    }
    else
    {
        close();
    }
}

void posix_chat_client::cb_read_input( const boost::system::error_code& error, std::size_t length )
{
    if( error )
    {
        if( error == boost::asio::error::not_found )
        {
            // didn't get a newline, wait for more data
            read_input();
            return;
        }
        
        std::cerr << "console error: " << error.message() << std::endl;
        close();
        return;
    }
    
    length = input_buffer_.size() - 1; // no newline
    
    // consume and write input_buffer_ to write_msg_
    input_buffer_.sgetn( write_msg_.data(), length );
    input_buffer_.consume( 1 ); // remove newline from input
    
    // populate m_msg
    m_msg.message = std::move( std::string( write_msg_.data(), write_msg_.data() + length ) );
    m_msg.nickname = m_nickname;
    //std::cout << m_msg.nickname << ": " << m_msg.message << std::endl;
    
    // msgpack m_msg and then send it
    msgpack::sbuffer sbuf;
    msgpack::pack( sbuf, m_msg );
    std::copy( sbuf.data(), sbuf.data() + sbuf.size(), write_msg_.begin() );
    
    auto buffers = boost::asio::buffer( write_msg_, sbuf.size() );
    auto handler = boost::bind( &posix_chat_client::cb_write_socket, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred );
    boost::asio::async_write( socket_, buffers, handler );
}

void posix_chat_client::cb_write_output( const boost::system::error_code& error, std::size_t bytes_written )
{
    if( !error )
    {
        read_socket();
    }
    else
    {
        close();
    }
}

void posix_chat_client::close()
{
    // cancel all outstanding asynchronous operations.
    socket_.close();
    input_.close();
    output_.close();
    
    //std::cout << "closing all connections" << std::endl;
}

bool posix_chat_client::feed_to_unpacker( const buffer_t& buffer, std::size_t length )
{
    try
    {
        // feed data to unpacker, need to decode incoming bytes
        // because we only want to send full messages
        m_unpacker.reserve_buffer( length );
        std::copy( buffer.data(), buffer.data() + length, m_unpacker.buffer() );
        m_unpacker.buffer_consumed( length );
        
        // maybe-get object out of it
        msgpack::unpacked result;
        
        if( m_unpacker.next( &result ) )
        {
            chat_message msg;
            result.get().convert( &m_msg );
        }
    }
    catch( std::bad_cast& e )
    {
        std::cerr << "server sent garbage, closing";
        close();
        return false;
    }
    
    return true;
}
