// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <deque>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

#include <msgpack.hpp>

#include "utils.hpp"

class chat_message
{
public:
    chat_message() {}
    
    std::string nickname;
    std::string msg;
    
    MSGPACK_DEFINE( nickname, msg );
};

class chat_participant
{
public:

    typedef std::shared_ptr<chat_participant> pointer;
    
    virtual ~chat_participant() {}
    virtual void deliver( const chat_message& msg ) = 0;
};

class chat_room
{
public:

    chat_room( std::string name );
    
    void join( chat_participant::pointer participant );
    void leave( chat_participant::pointer participant );
    void deliver( const chat_participant::pointer& sender, const chat_message& msg );
    
    friend std::ostream& operator<<( std::ostream& out, const chat_room& obj );
    
private:
    typedef std::set<chat_participant::pointer> member_set;
    member_set      m_members;
    std::string     m_name;
};

class chat_session
    : public chat_participant,
      public std::enable_shared_from_this<chat_session>
{
public:
    chat_session( tcp::socket socket, chat_room& room );
    
    void start();
    void deliver( const chat_message& msg );
    
    friend std::ostream& operator<<( std::ostream& out, const chat_session& obj );
    
private:
    void do_read();
    void do_write();
    
    tcp::socket socket_;
    chat_room& room_;
    
    typedef std::array<char, 1024> buffer_t;
    buffer_t m_read_buff;
    buffer_t m_write_buff;
    
    msgpack::unpacker m_unpacker;
    chat_message m_msg;
};

//----------------------------------------------------------------------

class chat_server
{
public:
    chat_server( boost::asio::io_service& io_service,
                 const tcp::endpoint& endpoint );
                 
    friend std::ostream& operator<<( std::ostream& out, const chat_server& obj );
    
private:
    void do_accept();
    
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    chat_room room_;
};
