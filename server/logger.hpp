#pragma once

#include <string>
#include <boost/log/common.hpp>

class Logger
{
public:

    enum severity_level
    {
        fatal,      // 0
        error,      // 1
        warning,    // 2
        info,       // 3
        debug,      // 4
        trace,      // 5
        decode      // 6
    };
    
    static Logger& instance()
    {
        static Logger instance;
        return instance;
    }
    
    void set_level( severity_level level );
    void enable_console();
    
    severity_level curr_level; // the current log level, used to shortcut potentially unnecessary "to_string()" calls
    
private:

    Logger();
    ~Logger();
    
};

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT( _logger, boost::log::sources::severity_logger<Logger::severity_level> )


// boost::log correctly shortcuts evaluating the input line if the set log level is higher than incoming message
#define TL_STREAM(level)    BOOST_LOG_SEV( _logger::get(), level ) << __FILE__ << ":" << __LINE__ << " "

#define TL_S_DECODE     TL_STREAM( Logger::decode )
#define TL_S_TRACE      TL_STREAM( Logger::trace )
#define TL_S_DEBUG      TL_STREAM( Logger::debug )
#define TL_S_INFO       TL_STREAM( Logger::info )
#define TL_S_WARNING    TL_STREAM( Logger::warning )
#define TL_S_WARN       TL_STREAM( Logger::warning )
#define TL_S_ERROR      TL_STREAM( Logger::error )
#define TL_S_FATAL      TL_STREAM( Logger::fatal )
