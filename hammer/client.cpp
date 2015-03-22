#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <iostream>

#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "client.hpp"

namespace asio = boost::asio;
namespace posix = boost::asio::posix;
using asio::ip::tcp;

#include <boost/thread/locks.hpp>
#include <boost/thread/lock_guard.hpp>
std::mutex cout_mutex;

bool hammer_client::keep_running = true;

hammer_client::hammer_client( asio::io_service& io_service,
                              tcp::resolver::iterator endpoint_iterator,
                              std::string nickname )
    : m_socket( io_service ),
      m_stdin( io_service, ::dup( STDIN_FILENO ) ),
      m_stdout( io_service, ::dup( STDOUT_FILENO ) ),
      m_input_buffer( max_msg_length )
{
    m_sent_count = 0;
    m_recv_count = 0;
    m_nickname = nickname;
    m_msg.nickname = m_nickname;

    // attempt to connect to server, call handle_connect when we do
    auto handler = boost::bind( &hammer_client::handle_connect, this, asio::placeholders::error );
    asio::async_connect( m_socket, endpoint_iterator, handler );
}

hammer_client::~hammer_client()
{
    std::lock_guard<std::mutex> lock( cout_mutex );
    std::cout << m_nickname << " sent " << m_sent_count << std::endl;
    std::cout << m_nickname << " recv " << m_recv_count << std::endl;
}

void hammer_client::handle_connect( const boost::system::error_code& error )
{
    if( error )
    {
        std::cerr << "could not connect to " << m_socket.remote_endpoint() << std::endl;
        return;
    }

    listen_on_socket();
    send_msg();
}

void hammer_client::listen_on_socket()
{
    // read from socket
    auto buffer = asio::buffer( m_read_buffer, 1024 );
    auto handler = boost::bind( &hammer_client::cb_read_socket, this, asio::placeholders::error, asio::placeholders::bytes_transferred );
    m_socket.async_read_some( buffer, handler );
}

void hammer_client::cb_read_socket( const boost::system::error_code& error, std::size_t bytes_recv )
{
    if( error )
    {
        std::cerr << "socket error: " << error.message() << std::endl;
        close();
        return;
    }

    m_recv_count++;
    listen_on_socket(); // read more bytes
}

void hammer_client::cb_write_socket( const boost::system::error_code& error, std::size_t length )
{
    if( error )
    {
        std::cerr << "socket write error: " << error.message() << std::endl;
        close();
        return;
    }

    send_msg();
}

typedef std::shared_ptr<boost::asio::deadline_timer> pointer_deadline_timer;

void hammer_client::send_msg()
{
    if( ! keep_running )
    {
        m_socket.get_io_service().stop();
        return;
    }

    pointer_deadline_timer timer = std::make_shared<boost::asio::deadline_timer>( m_socket.get_io_service() );
    timer->expires_from_now( boost::posix_time::milliseconds( 0 ) );
    timer->async_wait(
        [this, timer]( boost::system::error_code )
    {

        //std::chrono::duration<double, std::milli> t(1000);
        //std::this_thread::sleep_for(t);

        std::stringstream ss;
        ss << "msg num " << m_sent_count++;
        std::string message = ss.str();

        std::copy( message.begin(), message.end(), m_write_buffer.data() );

        // populate m_msg
        std::string& m = m_msg.message;
        m.replace( m.begin(), m.end(), message.begin(), message.end() );
        //std::cout << m_msg.nickname << ": " << m_msg.message << std::endl;

        // msgpack m_msg and then send it
        m_packer.clear();
        msgpack::pack( m_packer, m_msg );
        auto buffer = asio::buffer( m_packer.data(), m_packer.size() );
        auto handler = boost::bind( &hammer_client::cb_write_socket, this, asio::placeholders::error, asio::placeholders::bytes_transferred );
        asio::async_write( m_socket, buffer, handler );
    } );
}

void hammer_client::close()
{
    // cancel all outstanding asynchronous operations.
    m_socket.close();
}
