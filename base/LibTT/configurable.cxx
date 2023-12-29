#include <LibTT/configurable.h>
#include <LibLog/logging.h>

constexpr const char* LOGGING_PREFIX = "[Ozzy::ConfigLoader] ";

namespace Ozzy::Component
{
    bool Configurable::config_load(const std::string &config_path)
    {
        m_config = Filesystem::read_config_file(config_path);

        if (m_config.empty())
        {
            LibLog::log_print(LOGGING_PREFIX, "Error loading config file " + config_path);
            return false;
        }

        return true;
    }

    bool Configurable::config_contains(const std::vector<std::string> &data) const
    {
        for (auto &entry: data)
            if (!m_config.contains(entry))
            {
                LibLog::log_print(LOGGING_PREFIX, "When parsing config file, unable to find requested entry: " + entry);
                return false;
            }
        return true;
    }
}
