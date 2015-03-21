#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>

#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "client.hpp"

namespace asio = boost::asio;
namespace posix = boost::asio::posix;
using asio::ip::tcp;


posix_chat_client::posix_chat_client( asio::io_service& io_service,
                                      tcp::resolver::iterator endpoint_iterator,
                                      std::string nickname )
    : m_socket( io_service ),
      m_stdin( io_service, ::dup( STDIN_FILENO ) ),
      m_stdout( io_service, ::dup( STDOUT_FILENO ) ),
      m_input_buffer( max_msg_length )
{
    m_nickname = nickname;
    
    // attempt to connect to server, call handle_connect when we do
    auto handler = boost::bind( &posix_chat_client::handle_connect, this, asio::placeholders::error );
    asio::async_connect( m_socket, endpoint_iterator, handler );
}

void posix_chat_client::handle_connect( const boost::system::error_code& error )
{
    if( error )
    {
        std::cerr << "could not connect to " << m_socket.remote_endpoint();
        return;
    }
    
    read_socket();
    read_input();
}

void posix_chat_client::read_socket()
{
    // read from socket
    auto buffers = asio::buffer( m_read_buffer, 1024 );
    auto handler = boost::bind( &posix_chat_client::cb_read_socket, this, asio::placeholders::error, asio::placeholders::bytes_transferred );
    m_socket.async_read_some( buffers, handler );
}

void posix_chat_client::read_input()
{
    // read from console until newline
    auto handler = boost::bind( &posix_chat_client::cb_read_input, this, asio::placeholders::error, asio::placeholders::bytes_transferred );
    asio::async_read_until( m_stdin, m_input_buffer, '\n', handler );
}

void posix_chat_client::cb_read_socket( const boost::system::error_code& error, std::size_t bytes_recv )
{
    if( error )
    {
        std::cerr << "socket error: " << error.message() << std::endl;
        close();
        return;
    }
    
    if( feed_to_unpacker( m_read_buffer, bytes_recv ) )
    {
        static std::string output;

        std::stringstream ss;
        ss << m_msg.nickname << ": " << m_msg.message << std::endl;
        output = ss.str();
        
        // write out the message we just received, terminated by a newline.
        auto buffer = asio::buffer( output.data(), output.size() );
        asio::write( m_stdout, buffer );
    }

    read_socket(); // read more bytes
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
        if( error == asio::error::not_found )
        {
            // didn't get a newline, wait for more data
            read_input();
            return;
        }
        
        std::cerr << "console error: " << error.message() << std::endl;
        close();
        return;
    }
    
    length = m_input_buffer.size() - 1; // no newline
    
    // consume and write m_input_buffer to m_write_buffer
    m_input_buffer.sgetn( m_write_buffer.data(), length );
    m_input_buffer.consume( 1 ); // remove newline from input
    
    // populate m_msg
    m_msg.message = std::move( std::string( m_write_buffer.data(), m_write_buffer.data() + length ) );
    m_msg.nickname = m_nickname;
    //std::cout << m_msg.nickname << ": " << m_msg.message << std::endl;
    
    // msgpack m_msg and then send it
    m_packer.clear();
    msgpack::pack( m_packer, m_msg );
    auto buffers = asio::buffer( m_packer.data(), m_packer.size() );
    auto handler = boost::bind( &posix_chat_client::cb_write_socket, this, asio::placeholders::error, asio::placeholders::bytes_transferred );
    asio::async_write( m_socket, buffers, handler );
}

void posix_chat_client::cb_write_output( const boost::system::error_code& error, std::size_t bytes_written )
{
    if( error )
    {
        close();
        return;
    }

    read_socket();
}

void posix_chat_client::close()
{
    // cancel all outstanding asynchronous operations.
    m_socket.close();
    m_stdin.close();
    m_stdout.close();
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
