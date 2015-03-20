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

#include <boost/asio.hpp>

#include "logger.hpp"
#include "server.hpp"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

//----------------------------------------------------------------------

void chat_room::join( chat_participant_ptr participant )
{
    participants_.insert( participant );
    
    for( auto msg : recent_msgs_ )
        participant->deliver( msg );
}

void chat_room::leave( chat_participant_ptr participant )
{
    participants_.erase( participant );
}

void chat_room::deliver( const chat_message& msg )
{
    recent_msgs_.push_back( msg );
    
    while( recent_msgs_.size() > max_recent_msgs )
        recent_msgs_.pop_front();
        
    for( auto participant : participants_ )
        participant->deliver( msg );
}

//----------------------------------------------------------------------

chat_session::chat_session( tcp::socket socket, chat_room& room )
    : socket_( std::move( socket ) ),
      room_( room )
{
    TL_S_DEBUG << "creating chat_session in room: " << room;
}

void chat_session::start()
{
    TL_S_DEBUG << "chat_session start";
    room_.join( shared_from_this() );
    do_read_header();
}

void chat_session::deliver( const chat_message& msg )
{
    TL_S_TRACE << "deliver: " << msg;
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back( msg );
    
    if( !write_in_progress )
    {
        do_write();
    }
}

void chat_session::do_read_header()
{
    auto self( shared_from_this() );
    boost::asio::async_read( socket_,
                             boost::asio::buffer( read_msg_.data(), chat_message::header_length ),
                             [this, self]( boost::system::error_code ec, std::size_t /*length*/ )
    {
        if( !ec && read_msg_.decode_header() )
        {
            do_read_body();
        }
        else
        {
            room_.leave( shared_from_this() );
        }
    } );
}

void chat_session::do_read_body()
{
    auto self( shared_from_this() );
    boost::asio::async_read( socket_,
                             boost::asio::buffer( read_msg_.body(), read_msg_.body_length() ),
                             [this, self]( boost::system::error_code ec, std::size_t /*length*/ )
    {
        if( !ec )
        {
            room_.deliver( read_msg_ );
            do_read_header();
        }
        else
        {
            room_.leave( shared_from_this() );
        }
    } );
}

void chat_session::do_write()
{
    auto self( shared_from_this() );
    boost::asio::async_write( socket_,
                              boost::asio::buffer( write_msgs_.front().data(),
                                      write_msgs_.front().length() ),
                              [this, self]( boost::system::error_code ec, std::size_t /*length*/ )
    {
        if( !ec )
        {
            write_msgs_.pop_front();
            
            if( !write_msgs_.empty() )
            {
                do_write();
            }
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
      socket_( io_service )
{
    TL_S_DEBUG << "creating chat_server: " << endpoint;
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
            TL_S_DEBUG << "accepted connection";
            std::make_shared<chat_session>( std::move( socket_ ), room_ )->start();
        }
        
        do_accept();
    } );
}

    std::ostream& std::operator<<( std::ostream& out, const chat_message& obj )
{ return out; }
    std::ostream& std::operator<<( std::ostream& out, const chat_room& obj )
{ return out; }
    std::ostream& std::operator<<( std::ostream& out, const chat_session& obj )
{ return out; }
    std::ostream& std::operator<<( std::ostream& out, const chat_server& obj )
{ return out; }
