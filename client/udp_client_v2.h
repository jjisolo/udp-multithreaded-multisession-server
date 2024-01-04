#ifndef __UDP_CLIENT_V2__
#define __UDP_CLIENT_V2__

#include "udp_client_base.h"
#include "protocol.h"

namespace Ozzy::v2
{
    using boost::asio::ip::udp;

    class UdpClient : public Base::UdpClientBase
    {
        using server_answer_t = std::uint8_t;

    public:
        UdpClient(boost::asio::io_context &io_context, const std::string &config_path, const std::string logger_name, const double x)
            : UdpClientBase(io_context, config_path, logger_name, x)
        {
        }

    private:
        bool validate_protocol_versions() noexcept;

    public:
        void process_handshake() noexcept override;

        bool send_handshake()    noexcept override;
    };
}

#endif // __UDP_CLIENT_V2__
