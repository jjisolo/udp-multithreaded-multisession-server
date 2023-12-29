#include "networking.h"

namespace Ozzy::LibUDP
{
    template<typename T>
    bool receive_data(udp::endpoint& endpoint, udp::socket& socket, T& result, bool endian_swap)
    {
        std::size_t bytes_received = 0u;

        bytes_received = socket.receive_from(
                boost::asio::buffer(std::addressof(result), sizeof(T)),
                endpoint
        );

        if(endian_swap)
        {
            swap_endianess(result);
        }

        return bytes_received == sizeof(T);
    }

    template<typename T>
    bool send_data(udp::endpoint& endpoint, udp::socket& socket, T&& data, bool endian_swap)
    {
        std::size_t bytes_sended = 0u;

        if(endian_swap)
        {
            swap_endianess(data);
        }

        bytes_sended = socket.send_to(
            boost::asio::buffer(std::addressof(data), sizeof(T)),
            endpoint
        );

        return bytes_sended == sizeof(T);
    }

    template<typename T>
    void swap_endianess(T &data)
    {
        data = boost::endian::endian_reverse(data);
    }
}
