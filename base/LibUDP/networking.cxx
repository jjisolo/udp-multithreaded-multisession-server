#include "networking.h"
#include "LibLog/logging.h"

namespace Ozzy::LibUDP
{
    template<>
    void swap_endianess(double &data, bool to_big_endian)
    {
#if TARGET_DEVICE_LITTLE_ENDIAN
        if(!to_big_endian)
#else
        if(to_big_endian)
#endif
        {
            return;
        }

        std::uint64_t value = data;
        value = boost::endian::endian_reverse(value);
        data = value;
    }

    template<>
    void swap_endianess(std::array<std::uint8_t, Proto::Constant::TransmittionUnitSize> &data, bool to_big_endian)
    {
#if TARGET_DEVICE_LITTLE_ENDIAN
        if(!to_big_endian)
#else
        if(to_big_endian)
#endif
        {
            return;
        }

        const std::size_t array_size = data.size();
        for (std::size_t i = 0; i < array_size; ++i)
        {
            data[i] = boost::endian::endian_reverse(data[i]);
        }
    }

    template<>
    void swap_endianess(Proto::Handshake &handshake, bool to_big_endian)
    {
#if TARGET_DEVICE_LITTLE_ENDIAN
        if(!to_big_endian)
#else
        if(to_big_endian)
#endif
        {
            return;
        }

        std::uint8_t reversed_type;
        std::memcpy(&reversed_type, &handshake, 1);
        reversed_type = boost::endian::endian_reverse(handshake.type);
        std::memcpy(&handshake, &reversed_type, 1);
    }

    template<>
    void swap_endianess(Proto::Frame &frame, bool to_big_endian)
    {
#if TARGET_DEVICE_LITTLE_ENDIAN
        if(!to_big_endian)
#else
        if(to_big_endian)
#endif
        {
            return;
        }

        std::uint8_t reversed_type;
        std::memcpy(&reversed_type, &frame, 1);
        reversed_type = boost::endian::endian_reverse(frame.type);
        std::memcpy(&frame, &reversed_type, 1);

        frame.length   = boost::endian::endian_reverse(frame.length);
        frame.checksum = boost::endian::endian_reverse(frame.checksum);

        for (int i = 0; i < frame.length; ++i)
        {
            swap_endianess(frame.payload[i], to_big_endian);
        }
    }
}
