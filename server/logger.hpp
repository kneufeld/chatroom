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

// in TL_SEND it's a while to eliminate the possibility of interfering with an if/else that's not using brackets
#define TL_INSTANCE Logger::instance
#define TL_WRITE TL_INSTANCE().Write
#define TL_SEND(level, prefix, ... ) while( TL_INSTANCE().curr_level >= level ) { TL_WRITE( level, __FILE__, __LINE__, prefix, ##__VA_ARGS__ ); break; }

#define TL_DECODE( prefix, ... )    TL_SEND( Logger::decode,   prefix, ##__VA_ARGS__ );
#define TL_TRACE( prefix, ... )     TL_SEND( Logger::trace,    prefix, ##__VA_ARGS__ );
#define TL_DEBUG( prefix, ... )     TL_SEND( Logger::debug,    prefix, ##__VA_ARGS__ );
#define TL_INFO( prefix, ... )      TL_SEND( Logger::info,     prefix, ##__VA_ARGS__ );
#define TL_WARNING( prefix, ... )   TL_SEND( Logger::warning,  prefix, ##__VA_ARGS__ );
#define TL_WARN( prefix, ... )      TL_SEND( Logger::warning,  prefix, ##__VA_ARGS__ );
#define TL_ERROR( prefix, ... )     TL_SEND( Logger::error,    prefix, ##__VA_ARGS__ );
#define TL_FATAL( prefix, ... )     TL_SEND( Logger::fatal,    prefix, ##__VA_ARGS__ );

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
