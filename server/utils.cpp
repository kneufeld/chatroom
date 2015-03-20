
#include "utils.hpp"

boost::asio::io_service& IOS()
{
    static boost::asio::io_service ios;
    return ios;
}

