#include "logging.h"
#include <iostream>
#include <mutex>

namespace Ozzy::LibLog
{
    static std::mutex logging_mutex;

    void log_print(const std::string logger_name, const std::string message) noexcept
    {
        std::lock_guard<std::mutex> lock(logging_mutex);
        std::cerr << logger_name << message << std::endl;
    }

    std::string serialize_endpoint(const udp::endpoint endpoint)
    {
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }
}
