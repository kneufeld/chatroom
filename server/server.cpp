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
#include <boost/array.hpp>

#include "logger.hpp"
#include "server.hpp"

using boost::lexical_cast;
using boost::asio::ip::tcp;


chat_room::chat_room( std::string name )
{
    m_name = name;
}

void chat_room::join( chat_participant::pointer participant )
{
    m_members.insert( participant );
    TL_S_TRACE << *this << ": adding member, new length: " << m_members.size();
}

void chat_room::leave( chat_participant::pointer participant )
{
    m_members.erase( participant );
    TL_S_TRACE << *this << ": removing member, new length: " << m_members.size();
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
    do_read();
}

void chat_session::do_read()
{
    TL_S_TRACE << *this << ": listening to " << socket_.remote_endpoint();
    read_msg_length();
}

void chat_session::read_msg_length()
{
    auto self( shared_from_this() );
    boost::asio::async_read( socket_,
                             boost::asio::buffer( read_msg_.data(), 1 ),
                             [this, self]( boost::system::error_code ec, std::size_t length )
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
            
            room_.leave( self );
            return;
        }
        
        TL_S_TRACE << *this << ": received " << length << " bytes";

        unsigned msg_length = (unsigned)read_msg_[0];
        TL_S_DEBUG << *this << ": incoming msg will be " << msg_length << " bytes";

        read_msg_body(msg_length);
    });
}

void chat_session::read_msg_body( unsigned msg_length )
{
    auto self( shared_from_this() );
    boost::asio::async_read( socket_,
                             boost::asio::buffer( read_msg_.data(), msg_length ),
                             [this, self]( boost::system::error_code ec, std::size_t length )
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
            
            room_.leave( self );
            return;
        }
        
        TL_S_TRACE << *this << ": received " << length << " bytes";

        try
        {
            msgpack::zone zone;
            msgpack::object obj;
            msgpack::unpack( read_msg_.data(), length, NULL, &zone, &obj );
            obj.convert( &msg );
        }
        catch( std::bad_cast& e )
        {
            TL_S_ERROR << *this << ": recv'd bad message, closing connection";
            room_.leave(self);
            return;
        }
        
        TL_S_DEBUG << *this << ": msg from: " << msg.nickname; 
        room_.deliver( self, msg );
        
        do_read();
    } );
}

void chat_session::deliver( const chat_message& msg )
{
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, msg);

    memcpy( write_msg_.data(), sbuf.data(), sbuf.size() );
    do_write(sbuf.size());
}

void chat_session::do_write(unsigned msg_length)
{
    std::array<char,1> b = { (char)msg_length };
    boost::asio::write( socket_, boost::asio::buffer( b, 1 ) );

    auto self( shared_from_this() );
    boost::asio::async_write( socket_,
                              boost::asio::buffer( write_msg_.data(), msg_length ),
                              [this, self]( boost::system::error_code ec, std::size_t length )
    {
        if( !ec )
        {
            TL_S_TRACE << *self << ": wrote " << length << " bytes";
        }
        else
        {
            room_.leave( self );
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
            TL_S_INFO << "accepted connection from: " << socket_.remote_endpoint();
            std::make_shared<chat_session>( std::move( socket_ ), room_ )->start();
        }
        
        do_accept();
    } );
}

std::ostream& operator<<( std::ostream& out, const chat_message& obj )
{
    //out << obj.payload.length();
    return out;
}

std::ostream& operator<<( std::ostream& out, const chat_room& obj )
{
    out << "room(" << obj.m_name << ")";
    return out;
}

std::ostream& operator<<( std::ostream& out, const chat_session& obj )
{
    out << obj.room_ << "-" << obj.socket_.remote_endpoint();
    return out;
}

std::ostream& operator<<( std::ostream& out, const chat_server& obj )
{
    out << "chat_server: " << obj.acceptor_.local_endpoint();
    return out;
}
