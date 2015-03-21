#pragma once

#include <cstdlib>
#include <iostream>

#include <boost/asio.hpp>
#include <msgpack.hpp>

#include "common.hpp"

using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;

class posix_chat_client
{
public:
    posix_chat_client(
        boost::asio::io_service& io_service,
        tcp::resolver::iterator endpoint_iterator,
        std::string nickname );
        
private:

    void handle_connect( const boost::system::error_code& error );
    
    void read_socket();
    void cb_read_socket( const boost::system::error_code& error, std::size_t bytes_recv );
    void cb_write_socket( const boost::system::error_code& error, std::size_t length );
    
    void read_input();
    void cb_read_input( const boost::system::error_code& error, std::size_t length );
    void cb_write_output( const boost::system::error_code& error, std::size_t bytes_written );
    
    void close();
    
    bool feed_to_unpacker( const buffer_t& buffer, std::size_t length );
    
    tcp::socket socket_;
    posix::stream_descriptor input_;
    posix::stream_descriptor output_;
    buffer_t read_msg_;
    buffer_t write_msg_;
    boost::asio::streambuf input_buffer_;
    
    std::string m_nickname;
    msgpack::unpacker m_unpacker;
    chat_message m_msg;
};
