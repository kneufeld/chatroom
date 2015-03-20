
#include <iostream>
#include <boost/bind.hpp>

#include "utils.hpp"
#include "signals.hpp"

using namespace std;

SignalHandler::SignalHandler()
    : signals( IOS(), SIGINT, SIGTERM )
{
    wait_for_signal();
}

// stop the ios service when we get a term or ctrl-c
void SignalHandler::signal_handler( const boost::system::error_code& error, int signal_number )
{
    if( error )
    {
        // FIXME
        //TL_ERROR( "received signal handler error: %s. stopping tacar", error.message().c_str() );
        IOS().stop();
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

void SignalHandler::wait_for_signal()
{
    signals.async_wait(
        boost::bind( &SignalHandler::signal_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::signal_number )
    );
}

