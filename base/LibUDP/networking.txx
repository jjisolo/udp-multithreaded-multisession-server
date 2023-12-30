#include "networking.h"
#include <boost/system.hpp>

namespace Ozzy::LibUDP
{
    template<typename T>
    void swap_endianess(T &data, bool to_big_endian)
    {
#if TARGET_DEVICE_LITTLE_ENDIAN
        if(!to_big_endian)
#else
        if(to_big_endian)
#endif
        {
            return;
        }

        data = boost::endian::endian_reverse(data);
    }

    template<enum_type_t T>
    void swap_endianess(T &data, bool to_big_endian)
    {
#if TARGET_DEVICE_LITTLE_ENDIAN
        if(!to_big_endian)
#else
        if(to_big_endian)
#endif
        {
            return;
        }

        std::uint32_t value = data;
        value = boost::endian::endian_reverse(value);
        data = static_cast<T>(value);
    }

    template<typename T>
    bool receive_data(std::shared_ptr<Session>& session, T &result)
    {
        std::size_t bytes_received = 0u;
        boost::system::error_code error_code;

        bytes_received = session->socket.receive_from(
                boost::asio::buffer(std::addressof(result), sizeof(T)),
                session->endpoint
        );
        swap_endianess(result, session->to_big_endian);

        return bytes_received == sizeof(T);
    }

    template<typename T>
    bool send_data(std::shared_ptr<Session>& session, T &&data)
    {
        std::size_t bytes_sended = 0u;

        swap_endianess(data, session->to_big_endian);
        bytes_sended = session->socket.send_to(
            boost::asio::buffer(std::addressof(data), sizeof(T)),
            session->endpoint
        );

        return bytes_sended == sizeof(T);
    }
}
