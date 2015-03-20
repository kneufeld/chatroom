#pragma once

#include <vector>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace asio  = boost::asio;
//namespace tcp   = boost::asio::ip::tcp;
using boost::asio::ip::tcp;

asio::io_service& IOS();

class Server
{
public:

    typedef std::shared_ptr<Server> pointer;
    typedef std::vector<pointer> vector;
    
    Server( asio::io_service& ios, const tcp::endpoint& listen );
    ~Server();
    
private:

    void accept();
    
    tcp::socket     m_socket;
    tcp::acceptor   m_acceptor;
    tcp::endpoint   m_listen;
};
