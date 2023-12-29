#ifndef __OZZY_NETWORKING__
#define __OZZY_NETWORKING__

#include <boost/asio.hpp>

namespace Ozzy::LibUDP
{
    using boost::asio::ip::udp;

    template<typename T>
    bool receive_data(udp::endpoint& endpoint, udp::socket& socket, T& result);

    template<typename T>
    bool send_data(udp::endpoint& endpoint, udp::socket& socket, T&& data);
}

#include "networking.txx"

#endif //__OZZY_NETWORKING__
