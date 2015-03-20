#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <signal.h>
#include <time.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

//#include "logger.h"
#include "server.h"

const std::string app_name = "cr_server";
const unsigned max_num_ports = 5;

namespace po = boost::program_options;
namespace asio = boost::asio;
//namespace tcp   = boost::asio::ip::tcp;

using std::cout;
using std::cerr;
using std::endl;

struct SignalHandler
{
    SignalHandler()
        : signals( IOS(), SIGINT, SIGTERM )
    {
        wait_for_signal();
    }
    
    // stop the ios service when we get a term or ctrl-c
    void signal_handler( const boost::system::error_code& error, int signal_number )
    {
        if( error )
        {
            // FIXME
            //TL_ERROR( "received signal handler error: %s. stopping tacar", error.message().c_str() );
            //IOS().stop();
            return;
        }
        
        switch( signal_number )
        {
        case SIGUSR1: case SIGUSR2:
            //TL_INFO( "received signal: %d, ignoring", signal_number );
            wait_for_signal();
            break;
            
        case SIGHUP: case SIGINT: case SIGQUIT: case SIGKILL:
        default: // for now, just exit on any signal
            //TL_INFO( "received signal: %d, stopping tacar", signal_number );
            cout << "caught signal, exiting..." << endl;
            IOS().stop();
            break;
        }
    }
    
    void wait_for_signal()
    {
        signals.async_wait(
            boost::bind( &SignalHandler::signal_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::signal_number )
        );
    }
    
    boost::asio::signal_set signals;
};

bool parse_cmd_line( int argc, char** argv, po::variables_map& opts )
{
    po::options_description desc( app_name + " options" );
    
    desc.add_options()
    ( "help,h", "show help" )
    ( "debug,d", po::value<bool>()->implicit_value( true )->default_value( false ), "enable debug logging" )
    ( "ports", po::value<std::vector<unsigned> >(), "listen on ports" )
    ;
    
    po::positional_options_description pd;
    pd.add( "ports", -1 );
    
    try
    {
        auto parsed = po::command_line_parser( argc, argv ).options( desc ).positional( pd ).run();
        po::store( parsed, opts );
        po::notify( opts ); // this can throw
    }
    catch( boost::program_options::error& e )
    {
        cerr << e.what() << endl;
        return false;
    }
    catch( std::exception& e )
    {
        cerr << e.what() << endl;
        cerr << desc << endl;
        return false;
    }
    
    if( opts.count( "help" ) )
    {
        cout << "usage: " << app_name << " [options] " << pd.name_for_position( 0 ) << endl;
        cout << desc << endl;
        exit( 0 );
    }
    
    if( opts["debug"].as<bool>() )
    {
        //Logger::instance().set_level( Logger::debug );
    }
    
    if( ! opts.count( "ports" ) )
    {
        cerr << "must specifiy at least one port to listen on" << endl;
        return false;
    }
    
    return true;
}

/***********************************************************************
  Main Function
************************************************************************/
int main( int argc, char* argv[] )
{
    po::variables_map opts;
    
    if( ! parse_cmd_line( argc, argv, opts ) )
    {
        return 1;
    }
    
    Server::vector servers;
    
    for( auto port : opts["ports"].as< std::vector<unsigned> >() )
    {
        tcp::endpoint listen( tcp::v4(), port );
        servers.push_back( std::make_shared<Server>( IOS(), listen ) );
    }
        
    try
    {
        SignalHandler handler;
        IOS().run();
    }
    catch( std::exception& e )
    {
        std::cerr << "there was a catastrophic failure: " << e.what() << std::endl;
    }
    
    return 0;
}
