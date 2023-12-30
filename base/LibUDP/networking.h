#ifndef __OZZY_NETWORKING__
#define __OZZY_NETWORKING__

#include "protocol.h"
#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <utility>

namespace Ozzy::LibUDP
{
    using boost::asio::ip::udp;

    struct Session
    {
        Session(boost::asio::io_context &context, udp::endpoint endpoint)
            : socket(context), endpoint(std::move(endpoint)), to_big_endian(false)
        {
            socket.open(udp::v4());
            socket.bind(udp::endpoint(udp::v4(), 0));
        }

        explicit Session(boost::asio::io_context &context)
            : Session(context, udp::endpoint(udp::v4(), 0))
        {
        }

        void close()
        {
            socket.close();
        }

        udp::socket socket;
        udp::endpoint endpoint;
        bool to_big_endian;
    };

    template<typename T>
    bool receive_data(std::shared_ptr<Session> &session, T &result);

    template<typename T>
    bool send_data(std::shared_ptr<Session> &session, T &&data);


    template<typename T>
    concept enum_type_t = std::is_same_v<T, Proto::v1::Answer> ||
                          std::is_same_v<T, Proto::v2::Answer> ||
                          std::is_same_v<T, Proto::Version>;

    template<typename T>
    void swap_endianess(T &data, bool to_big_endian = false);

    template<enum_type_t T>
    void swap_endianess(T &data, bool to_big_endian);

    template<>
    void swap_endianess(double &data, bool to_big_endian);

    template<>
    void swap_endianess(std::array<std::uint8_t, Proto::Constant::TransmittionUnitSize> &data, bool to_big_endian);

    template<>
    void swap_endianess(Proto::Frame &frame, bool to_big_endian);

    template<>
    void swap_endianess(Proto::Handshake &handshake, bool to_big_endian);
}

#include "networking.txx"

#endif //__OZZY_NETWORKING__
