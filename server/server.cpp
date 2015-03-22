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

void chat_room::join( chat_session::pointer member )
{
    m_members.insert( member );
    TL_S_INFO << *this << ": adding member, new length: " << m_members.size();
}

void chat_room::leave( chat_session::pointer member )
{
    m_members.erase( member );
    TL_S_INFO << *this << ": removing member, new length: " << m_members.size();
}

void chat_room::deliver( chat_session::pointer sender, const chat_message& msg )
{
    for( auto member : m_members )
    {
        if( sender != member )
        {
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
    //TL_S_TRACE << *this << ": listening to " << socket_.remote_endpoint();

    auto self( shared_from_this() );

    socket_.async_read_some(
        boost::asio::buffer( m_read_buff.data(), 1024 ),
        [this, self]( boost::system::error_code ec, std::size_t length )
    {
        TL_S_TRACE << *this << ": recv'd " << length << " bytes";

        if( ec )
        {
            if( ec == boost::asio::error::operation_aborted )
            {
                TL_S_DEBUG << /**self <<*/ ": connection closed by us";
            }
            else if( ec == boost::asio::error::eof )
            {
                TL_S_DEBUG << /**self <<*/ ": connection closed by them";
            }
            else
            {
                TL_S_WARN << /**self <<*/ ": error: " << ec.message();
            }

            close();
            return;
        }

        try
        {
            // feed data to unpacker, need to decode incoming bytes
            // because we only want to send full messages
            m_unpacker.reserve_buffer( length );
            std::copy( m_read_buff.data(), m_read_buff.data() + length, m_unpacker.buffer() );
            m_unpacker.buffer_consumed( length );

            // maybe-get object out of it
            msgpack::unpacked result;

            if( m_unpacker.next( &result ) )
            {
                chat_message msg;
                result.get().convert( &msg );
                TL_S_DEBUG << *self << ": " << msg;

                room_.deliver( self, msg );
            }

            do_read();
        }
        catch( std::bad_cast& e )
        {
            TL_S_ERROR << *self << ": client sent garbage, dropping";
            close();
        }
    } );
}

void chat_session::deliver( const chat_message& msg )
{
    m_msg = msg;
    do_write();
}

void chat_session::do_write()
{
    TL_S_TRACE << *this <<  ": delivering";

    m_packer.clear();
    msgpack::pack( m_packer, m_msg );

    auto buffer = boost::asio::buffer( m_packer.data(), m_packer.size() );
    auto self( shared_from_this() );
    boost::asio::async_write( socket_, buffer,
                              [this, self]( boost::system::error_code ec, std::size_t length )
    {
        if( ec )
        {
            TL_S_ERROR << "do_write error: " << ec.message();
            close();
        }
        else
        {
            TL_S_TRACE << *self << ": wrote " << length << " bytes";
        }
    } );
}

void chat_session::close()
{
    auto self( shared_from_this() );
    room_.leave( self );
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

std::ostream& operator<<( std::ostream& out, const chat_room& obj )
{
    out << "room(" << obj.m_name << ")";
    return out;
}

std::ostream& operator<<( std::ostream& out, const chat_session& obj )
{
    //out << obj.room_;// << "-" << (obj.socket_.is_open() ? obj.socket_.remote_endpoint() : "");
    return out;
}

std::ostream& operator<<( std::ostream& out, const chat_server& obj )
{
    out << "chat_server: " << obj.acceptor_.local_endpoint();
    return out;
}
