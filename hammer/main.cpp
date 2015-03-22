#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>
#include <future>
#include <chrono>
#include <vector>
#include <algorithm>

#include <boost/asio.hpp>
using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;
using std::cout;
using std::cerr;
using std::endl;

#include "common.hpp"
#include "client.hpp"
#include "signals.hpp"

int main( int argc, char* argv[] )
{
    try
    {
        if( argc != 4 )
        {
            std::cerr << "Usage: chat_client <num_concurrent> <host> <port>\n";
            return 1;
        }

        boost::asio::io_service ios;
        SignalHandler signals( ios );

        unsigned num_concurrent = std::atoi( argv[1] );
        cout << "starting " << num_concurrent << " clients" << endl;

        std::vector<std::future<void>> futures;

        for( int i = 0; i < num_concurrent; ++i )
        {
            auto future = std::async( std::launch::async, [i, &argv]()
            {
                boost::asio::io_service io_service;

                tcp::resolver resolver( io_service );
                tcp::resolver::query query( argv[2], argv[3] );
                tcp::resolver::iterator iterator = resolver.resolve( query );

                std::stringstream ss;
                ss << "hammer-" << i;

                hammer_client c( io_service, iterator, ss.str() );
                io_service.run();
            } );
            futures.push_back( std::move( future ) );
        }

        ios.run();

        std::for_each( futures.begin(), futures.end(), []( std::future<void>& f )
        {
            f.wait();
        } );
    }
    catch( std::exception& e )
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
