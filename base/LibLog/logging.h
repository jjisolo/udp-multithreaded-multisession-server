#ifndef __OZZY_LOGGING_H__
#define __OZZY_LOGGING_H__

#include <string>
#include <boost/asio.hpp>

namespace Ozzy::LibLog
{
    using boost::asio::ip::udp;

    void log_print(std::string logger_name, std::string message) noexcept;

    std::string serialize_endpoint(udp::endpoint endpoint);
}

#endif //__OZZY_LOGGING_H__
