#ifndef __OZZY_UDP_SERVER_V2__
#define __OZZY_UDP_SERVER_V2__

#include <boost/asio.hpp>
#include "LibFS/filesystem.h"
#include "protocol.h"
#include "udp_server_base.h"

namespace Ozzy::v2
{
    using boost::asio::ip::udp;

    class UdpServer : public Base::UdpServerBase
    {
        using client_answer_t = std::uint_fast8_t;

    public:
        UdpServer(boost::asio::io_context &io_context, const std::string &config_path, const std::string &logger_name,
                  const std::uint64_t doubles_count)
            : UdpServerBase(io_context, config_path, logger_name, doubles_count)
        {
        }

    protected:
        // Handle the received message
        void handle_message(udp::endpoint client_endpoint, std::size_t bytes_received) noexcept override;

        // Handle the handshake between the server and the client
        void handle_handshake(std::shared_ptr<LibUDP::Session>&& session) noexcept override;

        // Send individual frame to the client
        bool send_frame(std::shared_ptr<LibUDP::Session>& session, Proto::Frame frame) noexcept override;

        // Send array of frames with random doubles from -x to x
        bool send_frame_array(std::shared_ptr<LibUDP::Session>& session, double x) noexcept override;
    };
}

#endif // __OZZY_UDP_SERVER_V2__
