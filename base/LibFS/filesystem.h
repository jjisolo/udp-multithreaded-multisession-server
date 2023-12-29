#ifndef __OZZ_FILESYSTEM__
#define __OZZ_FILESYSTEM__

#include <string>
#include <filesystem>
#include <unordered_map>

#include "protocol.h"

namespace Ozzy::Filesystem
{
    std::unordered_map<std::string, std::string> read_config_file(const std::string& path) noexcept;
}

#endif // __OZZ_FILESYSTEM__
