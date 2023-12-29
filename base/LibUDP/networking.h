#ifndef __OZZY_NETWORKING__
#define __OZZY_NETWORKING__

#include "protocol.h"
#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>

namespace Ozzy::LibUDP
{
    using boost::asio::ip::udp;

    template<typename T>
    bool receive_data(udp::endpoint &endpoint, udp::socket &socket, T &result, bool endian_swap = false);

    template<typename T>
    bool send_data(udp::endpoint &endpoint, udp::socket &socket, T &&data, bool endian_swap = false);

    template<typename T>
    void swap_endianess(T &data);

    template<>
    void swap_endianess(double &data);

    template<>
    void swap_endianess(Proto::v1::Answer &data);

    template<>
    void swap_endianess(Proto::v2::Answer &data);

    template<>
    void swap_endianess(Proto::Version &data);

    template<>
    void swap_endianess(std::array<std::uint8_t, Proto::Constant::TransmittionUnitSize> &data);

    template<>
    void swap_endianess(Proto::Frame &frame);

    template<>
    void swap_endianess(Proto::Handshake &handshake);
}

#include "networking.txx"

#endif //__OZZY_NETWORKING__
