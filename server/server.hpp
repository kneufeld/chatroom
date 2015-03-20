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

#include <msgpack.hpp>

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
    
    friend std::ostream& operator<<( std::ostream& out, const chat_message& obj );

private:
    char data_[header_length + max_body_length];
    std::size_t body_length_;
};

class chat_participant
{
public:

    typedef std::shared_ptr<chat_participant> pointer;
    
    virtual ~chat_participant() {}
    virtual void deliver( const chat_message& msg ) = 0;
};


//----------------------------------------------------------------------

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
    void do_read_header();
    void do_write();
    
    tcp::socket socket_;
    chat_room& room_;
    chat_message read_msg_;
    chat_message write_msg_;
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
