#include "networking.h"

namespace Ozzy::LibUDP
{
    template<typename T>
    bool receive_data(udp::endpoint& endpoint, udp::socket& socket, T& result)
    {
        std::size_t bytes_received = 0u;

        bytes_received = socket.receive_from(
                boost::asio::buffer(std::addressof(result), sizeof(T)),
                endpoint
        );

        return bytes_received == sizeof(T);
    }

    template<typename T>
    bool send_data(udp::endpoint& endpoint, udp::socket& socket, T&& data)
    {
        std::size_t bytes_sended = 0u;

        bytes_sended = socket.send_to(
            boost::asio::buffer(std::addressof(data), sizeof(T)),
            endpoint
        );

        return bytes_sended == sizeof(T);
    }
}
