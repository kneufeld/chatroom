#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/date_time.hpp>
#include <boost/log/support/date_time.hpp>

#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/log/expressions/formatter.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace logging	= boost::log;
namespace src		= boost::log::sources;
namespace sinks		= boost::log::sinks;
namespace keywords	= boost::log::keywords;
namespace attrs   	= boost::log::attributes;
namespace expr    	= boost::log::expressions;

#include "logger.hpp"

// for more info on boost logging:
// http://www.boost.org/doc/libs/1_56_0/libs/log/doc/html/index.html
// http://www.boost.org/doc/libs/1_56_0/libs/log/doc/html/log/detailed/attributes.html#log.detailed.attributes.named_scope

#define LFC1_LOG(_logger, level) BOOST_LOG_SEV(_logger, level) << __FILE__ << ":" << __LINE__ << " "

#define LFC1_LOG_TRACE(_logger) LFC1_LOG(_logger, Logger::trace)
#define LFC1_LOG_DEBUG(_logger) LFC1_LOG(_logger, Logger::debug)
#define LFC1_LOG_INFO(_logger) LFC1_LOG(_logger, Logger::info)
#define LFC1_LOG_WARNING(_logger) LFC1_LOG(_logger, Logger::warning)
#define LFC1_LOG_ERROR(_logger) LFC1_LOG(_logger, Logger::error)

BOOST_LOG_ATTRIBUTE_KEYWORD( scope, "Scope", attrs::named_scope::value_type )
BOOST_LOG_ATTRIBUTE_KEYWORD( process_id, "ProcessID", attrs::current_process_id::value_type )
BOOST_LOG_ATTRIBUTE_KEYWORD( timestamp, "TimeStamp", attrs::local_clock::value_type )
//BOOST_LOG_ATTRIBUTE_KEYWORD( timestamp, "TimeStamp", boost::posix_time::microsec_clock )
BOOST_LOG_ATTRIBUTE_KEYWORD( thread_id, "ThreadID", attrs::current_thread_id::value_type )
BOOST_LOG_ATTRIBUTE_KEYWORD( severity, "Severity", Logger::severity_level )

template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
    std::basic_ostream< CharT, TraitsT >& stream, Logger::severity_level lvl )
{
    static const char* const names[] =
    {
        "Fatal",
        "Error",
        "Warning",
        "Info",
        "Debug",
        "Trace",
        "decode",
        "" // safety
    };

    stream << names[lvl][0];
    return stream;
}

template<typename T>
void set_formatter( T& sink )
{
    // this is the incredibly complicated magic that actually makes the output line
    sink->set_formatter(
        expr::stream
        << expr::if_( severity <= Logger::warning )[ expr::stream << "*" << severity ].else_[ expr::stream << " " << severity ]
        << ": "
        << expr::format_date_time( timestamp, "%H:%M:%S.%f" ) << " "
        << expr::smessage // the LFC1_LOG macro prepends __FILE__, __LINE__ to actual message
    );
}

template<typename S>
void mk_stream_logger( S stream )
{
    // Create a backend and attach a couple of streams to it
    typedef sinks::text_ostream_backend backend_t;
    boost::shared_ptr<backend_t> backend = boost::make_shared<backend_t>();

    backend->add_stream( stream );
    backend->auto_flush( true ); // enable auto-flushing after each log record written

    typedef sinks::synchronous_sink<backend_t> sink_t;
    boost::shared_ptr< sink_t > sink = boost::make_shared<sink_t>( backend );

    set_formatter( sink );

    logging::core::get()->add_sink( sink );
}

Logger::Logger()
{
    // Add some attributes
    logging::add_common_attributes(); // TimeStamp, etc. http://boost-log.sourceforge.net/libs/log/doc/html/boost/log/add_common_attributes.html

    boost::shared_ptr< logging::core > core = logging::core::get();
    core->add_global_attribute( "Scope", attrs::named_scope() );

    // boost::log has a default sink that you can't delete, the only way to not log
    // anything to the console is to disable logging and then re-enable when any
    // cmdline option sets a logging sink
    core->set_logging_enabled( false );

    set_level( Logger::info );
    enable_console();
}

Logger::~Logger()
{

}

void Logger::set_level( severity_level level )
{
    if( level < fatal || level > decode )
    {
        LFC1_LOG_ERROR( _logger::get() ) << "trying to set invalid log level: " << ( int )level;
        return;
    }

    LFC1_LOG_INFO( _logger::get() ) << "setting log level to: " << level;
    curr_level = level;
    logging::core::get()->set_filter( severity <= level );
}

void Logger::enable_console()
{
    logging::core::get()->set_logging_enabled( true );
    mk_stream_logger( boost::shared_ptr< std::ostream >( &std::cout, boost::null_deleter() ) );
    LFC1_LOG_INFO( _logger::get() ) << "sending logs to console";
}
