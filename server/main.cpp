#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>

#include <signal.h>
#include <time.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include "logger.hpp"
#include "signals.hpp"
#include "server.hpp"

const std::string app_name = "server";
const unsigned max_num_ports = 5;

namespace po = boost::program_options;

using std::cout;
using std::cerr;
using std::endl;

bool parse_cmd_line( int argc, char** argv, po::variables_map& opts )
{
    po::options_description desc( app_name + " options" );
    
    desc.add_options()
    ( "help,h", "show help" )
    ( "debug,d", po::value<unsigned>()->implicit_value( Logger::debug )->default_value( Logger::info ), "enable debug logging" )
    ( "ports", po::value<std::vector<unsigned> >()->required(), "listen on ports" )
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
    
    Logger::instance().set_level( ( Logger::severity_level )opts["debug"].as<unsigned>() );

    return true;
}

int main( int argc, char* argv[] )
{
    po::variables_map opts;
    
    if( ! parse_cmd_line( argc, argv, opts ) )
    {
        return 1;
    }
    
    try
    {
        boost::asio::io_service ios;

        SignalHandler handler(ios);
        std::list<chat_server> servers;
        
        for( auto port : opts["ports"].as< std::vector<unsigned> >() )
        {
            tcp::endpoint endpoint( tcp::v4(), port );
            servers.emplace_back( ios, endpoint );
        }
        
        ios.run();
    }
    catch( std::exception& e )
    {
        cerr << "there was a catastrophic failure: " << e.what() << endl;
    }
    
    return 0;
}
