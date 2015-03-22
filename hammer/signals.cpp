
#include <boost/bind.hpp>

#include "client.hpp"
#include "signals.hpp"

using namespace std;

SignalHandler::SignalHandler( boost::asio::io_service& ios )
    : signals( ios, SIGINT, SIGTERM )
{
    wait_for_signal();
}

// stop the ios service when we get a term or ctrl-c
void SignalHandler::signal_handler( const boost::system::error_code& error, int signal_number )
{
    if( error )
    {
        signals.get_io_service().stop();
        return;
    }

    switch( signal_number )
    {
    case SIGUSR1: case SIGUSR2:
        wait_for_signal();
        break;

    case SIGHUP: case SIGINT: case SIGQUIT: case SIGKILL:
    default: // for now, just exit on any signal
        hammer_client::keep_running = false;
        signals.get_io_service().stop();
        break;
    }
}

void SignalHandler::wait_for_signal()
{
    signals.async_wait(
        boost::bind( &SignalHandler::signal_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::signal_number )
    );
}

