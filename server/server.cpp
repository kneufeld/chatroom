//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <algorithm>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "logger.hpp"
#include "server.hpp"

using boost::lexical_cast;
using boost::asio::ip::tcp;

//----------------------------------------------------------------------

//----------------------------------------------------------------------

chat_room::chat_room( std::string name )
{
    m_name = name;
}

void chat_room::join( chat_participant::pointer participant )
{
    m_members.insert( participant );
}

void chat_room::leave( chat_participant::pointer participant )
{
    m_members.erase( participant );
}

void chat_room::deliver( const chat_participant::pointer& sender, const chat_message& msg )
{
    for( auto member : m_members )
    {
        if( sender != member )
        {
            TL_S_TRACE << *this <<  ": delivering";
            member->deliver( msg );
        }
    }
}

//----------------------------------------------------------------------

chat_session::chat_session( tcp::socket socket, chat_room& room )
    : socket_( std::move( socket ) ),
      room_( room )
{
    TL_S_DEBUG << "creating " << *this;
}

void chat_session::start()
{
    TL_S_DEBUG << *this << ": started";
    room_.join( shared_from_this() );
    do_read_header();
}

void chat_session::do_read_header()
{
    auto self( shared_from_this() );
    boost::asio::async_read( socket_,
                             boost::asio::buffer( read_msg_.data(), chat_message::header_length ),
                             [this, self]( boost::system::error_code ec, std::size_t /*length*/ )
    {
        if( ec )
        {
            if( ec == boost::asio::error::operation_aborted )
            {
                TL_S_DEBUG << *this << ": connection closed by us";
            }
            else if( ec == boost::asio::error::eof )
            {
                TL_S_DEBUG << *this << ": connection closed by them";
            }
            else
            {
                TL_S_WARN << *this << ": error: " << ec.message();
            }
            
            room_.leave( shared_from_this() );
            return;
        }
        else
        {
            TL_S_TRACE << *this << ": got data: " << read_msg_.data();
            room_.deliver( self, read_msg_ );
        }
        do_read_header();
    } );
}

void chat_session::deliver( const chat_message& msg )
{
    write_msg_ = msg;
    do_write();
}

void chat_session::do_write()
{
    auto self( shared_from_this() );
    boost::asio::async_write( socket_,
                              boost::asio::buffer( write_msg_.data(), write_msg_.length() ),
                              [this, self]( boost::system::error_code ec, std::size_t /*length*/ )
    {
        if( !ec )
        {
            TL_S_TRACE << *self << ": wrote " << write_msg_.length() << " bytes";
        }
        else
        {
            room_.leave( shared_from_this() );
        }
    } );
}

//----------------------------------------------------------------------

chat_server::chat_server( boost::asio::io_service& io_service,
                          const tcp::endpoint& endpoint )
    : acceptor_( io_service, endpoint ),
      socket_( io_service ),
      room_( lexical_cast<std::string>( endpoint.port() ) )
{
    TL_S_DEBUG << "creating: " << *this;
    do_accept();
}

void chat_server::do_accept()
{
    acceptor_.async_accept( socket_,
                            [this]( boost::system::error_code ec )
    {
        if( ec )
        {
            TL_S_ERROR << "accept error: " << ec.message();
        }
        else
        {
            TL_S_DEBUG << "accepted connection from: " << socket_.remote_endpoint();
            std::make_shared<chat_session>( std::move( socket_ ), room_ )->start();
        }
        
        do_accept();
    } );
}

std::ostream& operator<<( std::ostream& out, const chat_message& obj )
{
    out << obj.data_;
    return out;
}

std::ostream& operator<<( std::ostream& out, const chat_room& obj )
{
    out << obj.m_name;
    return out;
}

std::ostream& operator<<( std::ostream& out, const chat_session& obj )
{
    out << "chat_session(" << obj.room_ << ") " << obj.socket_.remote_endpoint();
    return out;
}

std::ostream& operator<<( std::ostream& out, const chat_server& obj )
{
    out << "chat_server: " << obj.acceptor_.local_endpoint();
    return out;
}
