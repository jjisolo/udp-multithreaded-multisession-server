#ifndef __OZZY_COMPONENT_CONFIGURABLE__
#define __OZZY_COMPONENT_CONFIGURABLE__

#include <string>
#include <vector>
#include <LibFS/filesystem.h>


namespace Ozzy::Component
{
    class Configurable
    {
    protected:
        Configurable(): m_config({})
        {
        }

        // Load the configuration file
        bool config_load(const std::string &config_path);

        // Assert that the config contains provided values
        bool config_contains(const std::vector<std::string> &data) const;

    protected:
        std::unordered_map<std::string, std::string> m_config;
    };
}

#endif // __OZZY_COMPONENT_CONFIGURABLE__
