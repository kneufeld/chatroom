#pragma once

#include <cstdlib>
#include <iostream>

#include <boost/asio.hpp>
#include <msgpack.hpp>

#include "common.hpp"

using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;

class hammer_client
{
public:
    hammer_client(
        boost::asio::io_service& io_service,
        tcp::resolver::iterator endpoint_iterator,
        std::string nickname );

    ~hammer_client();

    static bool keep_running;

private:

    void handle_connect( const boost::system::error_code& error );

    void listen_on_socket();
    void cb_read_socket( const boost::system::error_code& error, std::size_t bytes_recv );
    void cb_write_socket( const boost::system::error_code& error, std::size_t length );

    void close();

    void send_msg();

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

    unsigned long m_sent_count;
    unsigned long m_recv_count;
};
