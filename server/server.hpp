//
// chat_message.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <deque>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

#include "utils.hpp"

class chat_message
{
public:
    enum { header_length = 4 };
    enum { max_body_length = 512 };
    
    chat_message()
        : body_length_( 0 )
    {
    }
    
    const char* data() const
    {
        return data_;
    }
    
    char* data()
    {
        return data_;
    }
    
    std::size_t length() const
    {
        return header_length + body_length_;
    }
    
    const char* body() const
    {
        return data_ + header_length;
    }
    
    char* body()
    {
        return data_ + header_length;
    }
    
    std::size_t body_length() const
    {
        return body_length_;
    }
    
    void body_length( std::size_t new_length )
    {
        body_length_ = new_length;
        
        if( body_length_ > max_body_length )
            body_length_ = max_body_length;
    }
    
    bool decode_header()
    {
        char header[header_length + 1] = "";
        std::strncat( header, data_, header_length );
        body_length_ = std::atoi( header );
        
        if( body_length_ > max_body_length )
        {
            body_length_ = 0;
            return false;
        }
        
        return true;
    }
    
    void encode_header()
    {
        char header[header_length + 1] = "";
        std::sprintf( header, "%4d", static_cast<int>( body_length_ ) );
        std::memcpy( data_, header, header_length );
    }
    
private:
    char data_[header_length + max_body_length];
    std::size_t body_length_;
};

class chat_participant
{
public:
    virtual ~chat_participant() {}
    virtual void deliver( const chat_message& msg ) = 0;
};

typedef std::deque<chat_message> chat_message_queue;
typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
    void join( chat_participant_ptr participant );
    void leave( chat_participant_ptr participant );
    void deliver( const chat_message& msg );
    
private:
    std::set<chat_participant_ptr> participants_;
    enum { max_recent_msgs = 100 };
    chat_message_queue recent_msgs_;
};

class chat_session
    : public chat_participant,
      public std::enable_shared_from_this<chat_session>
{
public:
    chat_session( tcp::socket socket, chat_room& room );
    
    void start();
    void deliver( const chat_message& msg );
    
private:
    void do_read_header();
    void do_read_body();
    void do_write();
    
    tcp::socket socket_;
    chat_room& room_;
    chat_message read_msg_;
    chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class chat_server
{
public:
    chat_server( boost::asio::io_service& io_service,
                 const tcp::endpoint& endpoint );
                 
private:
    void do_accept();
    
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    chat_room room_;
};

namespace std {
    std::ostream& operator<<( std::ostream& out, const chat_message& obj );
    std::ostream& operator<<( std::ostream& out, const chat_room& obj );
    std::ostream& operator<<( std::ostream& out, const chat_session& obj );
    std::ostream& operator<<( std::ostream& out, const chat_server& obj );
}
