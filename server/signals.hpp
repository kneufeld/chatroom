#pragma once

#include <signal.h>
#include <boost/asio.hpp>

class SignalHandler
{
public:

    SignalHandler( boost::asio::io_service& ios );
    
private:

    // stop the ios service when we get a term or ctrl-c
    void signal_handler( const boost::system::error_code& error, int signal_number );
    void wait_for_signal();
    
    boost::asio::signal_set signals;
};

