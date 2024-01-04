#ifndef __OZZY_UDP_CLIENT_BASE__
#define __OZZY_UDP_CLIENT_BASE__

#include <string>
#include <boost/asio.hpp>
#include "LibLog/logging.h"
#include "LibUDP/networking.h"
#include "LibTT/configurable.h"
#include "LibTT/informative.h"

namespace Ozzy::Base
{
    using boost::asio::ip::udp;

    class UdpClientBase : public Component::Configurable, public Component::Informative
    {
    public:
        UdpClientBase(boost::asio::io_context &io_context, const std::string &config_path, const std::string logger_name, const double x)
            : Informative(logger_name), m_io_context(io_context), m_upper_bound(x)
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

            try
            {
                auto resolver  = udp::resolver(io_context);
                auto endpoints = resolver.resolve(udp::v4(), m_config["server_ip_address"], m_config["server_port"]);
		
                m_session = std::make_shared<LibUDP::Session>(io_context);
                // Perform none endian conversions, because server has already handled
                // it for us
#if TARGET_DEVICE_LITTLE_ENDIAN
                m_session->to_big_endian = false;
#else
                m_session->to_big_endian = true;
#endif
                m_session->endpoint = *endpoints.begin();
                m_session->socket   = udp::socket(io_context);
                m_session->socket.open(udp::v4());
                m_session->socket.bind(udp::endpoint(udp::v4(), 0));
            }
            catch (const boost::system::system_error &)
            {
                LibLog::log_print(m_logger_name, "Unable to initialize udp socket");
                return;
            }
        }

        virtual ~UdpClientBase();

        UdpClientBase(const UdpClientBase&)            = delete;

        UdpClientBase& operator=(const UdpClientBase&) = delete;

    public:
        virtual void process_handshake() noexcept = 0;

        virtual bool send_handshake()    noexcept = 0;

    protected:
        std::shared_ptr<LibUDP::Session> m_session;
        boost::asio::io_context         &m_io_context;
        double                           m_upper_bound;
    };
}

#endif // __OZZY_UDP_CLIENT_BASE__
