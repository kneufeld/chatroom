#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <thread>

#include <boost/asio.hpp>
using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;

#include "common.hpp"
#include "client.hpp"

int main( int argc, char* argv[] )
{
    try
    {
        if( argc != 4 )
        {
            std::cerr << "Usage: chat_client <nickname> <host> <port>\n";
            return 1;
        }
        
        boost::asio::io_service io_service;
        
        tcp::resolver resolver( io_service );
        tcp::resolver::query query( argv[2], argv[3] );
        tcp::resolver::iterator iterator = resolver.resolve( query );
        
        posix_chat_client c( io_service, iterator, argv[1] );
        
        std::cout << "start typing..." << std::endl;
        io_service.run();
    }
    catch( std::exception& e )
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    return 0;
}
