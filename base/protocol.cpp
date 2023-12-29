#include "protocol.h"

namespace Ozzy::Proto
{
    std::uint64_t calculate_frame_checksum(Frame frame)
    {
        const std::uint8_t *data    = reinterpret_cast<const std::uint8_t *>(frame.payload);
        const std::size_t data_size = frame.length / sizeof(double);

        std::uint64_t checksum = 0;
        for(std::size_t i = 0; i < data_size * sizeof(double); ++i)
        {
            checksum ^= data[i];
        }

        return checksum & ((1ULL << FRAME_BITS_PER_CHECKSUM) - 1);
    }
}