#include "filesystem.h"

#include <LibLog/logging.h>
#include <fstream>
#include <random>
#include <mutex>

static constexpr const char*   LOGGING_PREFIX       = "[Ozzy::Filesystem] ";


namespace Ozzy::Filesystem
{
    std::unordered_map<std::string, std::string> read_config_file(const std::string &path) noexcept
    {
        std::unordered_map<std::string, std::string> result = {};

        try
        {
            LibLog::log_print(LOGGING_PREFIX, "Reading config file " + path);

            if (!std::filesystem::exists(path))
            {
                LibLog::log_print(LOGGING_PREFIX, "Config file " + path + "does not exists");
                return {};
            }

            std::ifstream file_stream = std::ifstream(path.data());
            if (!file_stream)
            {
                LibLog::log_print(LOGGING_PREFIX, "Config file " + path + "does not exists");
                return {};
            }

            // Parse the file contents
            std::istringstream iss;
            std::string line;
            std::string variable_name, variable_value;

            while (std::getline(file_stream, line))
            {
                iss = std::istringstream(line);

                if (iss >> variable_name >> variable_value)
                {
                    result[variable_name] = variable_value;
                }
            }

            file_stream.close();
            return result;
        }
        catch (const std::exception &e)
        {
            LibLog::log_print(LOGGING_PREFIX, "Exception triggered: " + std::string(e.what()));
        }
        catch (...)
        {
            LibLog::log_print(LOGGING_PREFIX, "Unknown exception has been triggered");
        }

        return {};
    }
}
