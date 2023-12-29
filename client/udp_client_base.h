#ifndef __OZZY_UDP_CLIENT_BASE__
#define __OZZY_UDP_CLIENT_BASE__

#include <string>
#include <mutex>
#include <boost/asio.hpp>
#include "protocol.h"
#include "LibLog/logging.h"
#include "LibTT/configurable.h"
#include "LibTT/informative.h"

namespace Ozzy::Base
{
    using boost::asio::ip::udp;

    class UdpClientBase : public Component::Configurable, public Component::Informative
    {
    public:
        UdpClientBase(boost::asio::io_context &io_context, const std::string &config_path,
                      const std::string &logger_name, const double x)
            : Informative(logger_name), m_socket(io_context), m_io_context(io_context), m_upper_bound(x)
        {
            if (!config_load(config_path))
            {
                LibLog::log_print(m_logger_name, "Failed starting client(error loading config file)");
                return;
            }

            if (!config_contains({"server_port", "server_ip_address"}))
            {
                LibLog::log_print(m_logger_name, "Failed starting client(error parsing config file)");
                return;
            }

            auto resolver = udp::resolver(io_context);
            auto endpoints = resolver.resolve(udp::v4(), m_config["server_ip_address"], m_config["server_port"]);
            m_endpoint = *endpoints.begin();

            try
            {
                m_socket.open(udp::v4());
                m_socket.bind(udp::endpoint(udp::v4(), 0));
            }
            catch (const boost::system::system_error &)
            {
                LibLog::log_print(m_logger_name, "Unable to initialize udp socket");
                return;
            }
        }

        virtual ~UdpClientBase();

    protected:
        virtual void write_frame(Proto::Frame &frame);

    public:
        virtual void process_handshake() = 0;

        virtual bool send_handshake() = 0;

    protected:
        udp::socket m_socket;
        udp::endpoint m_endpoint;
        std::mutex m_send_mutex;
        std::mutex m_recv_mutex;
        double m_upper_bound;
        boost::asio::io_context &m_io_context;
    };
}

#endif // __OZZY_UDP_CLIENT_BASE__
