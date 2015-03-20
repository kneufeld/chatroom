
#include <iostream>

#include "server.h"

using namespace std;

asio::io_service& IOS()
{
    static asio::io_service ios;
    return ios;
}

Server::Server( asio::io_service& ios, const tcp::endpoint& listen )
    : m_socket( ios ), m_acceptor( ios, listen ), m_listen(listen)
{
    cout << "creating server: " << m_listen << endl;
    accept();
}

Server::~Server()
{
    cout << "deleting server for port: " << m_listen << endl;
}

void Server::accept()
{
    m_acceptor.async_accept( m_socket,
                             [this]( boost::system::error_code ec )
    {
        if( ec )
        {
            cerr << "error accepting: " << ec.message();
        }
        else
        {
            //std::make_shared<chat_session>( std::move( socket_ ), room_ )->start();
            cout << "accepted connection" << endl;
        }
        
        accept();
    } );
}
