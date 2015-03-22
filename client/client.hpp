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

    void listen_on_socket();
    void cb_read_socket( const boost::system::error_code& error, std::size_t bytes_recv );
    void cb_write_socket( const boost::system::error_code& error, std::size_t length );

    void listen_on_input();
    void cb_read_input( const boost::system::error_code& error, std::size_t length );

    void close();

    bool feed_to_unpacker( const buffer_t& buffer, std::size_t length );

    tcp::socket m_socket;
    posix::stream_descriptor m_stdin;
    posix::stream_descriptor m_stdout;
    buffer_t m_read_buffer;
    buffer_t m_write_buffer;
    boost::asio::streambuf m_input_buffer;

    std::string m_nickname;
    msgpack::unpacker m_unpacker;
    msgpack::sbuffer  m_packer;
    chat_message m_msg;
};
