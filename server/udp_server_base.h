#ifndef __OZZY_UDP_SERVER_BASE__
#define __OZZY_UDP_SERVER_BASE__

#include <stdexcept>
#include <random>
#include <vector>
#include <boost/asio.hpp>

#include "protocol.h"
#include "LibLog/logging.h"
#include "LibUDP/networking.h"
#include "LibTT/configurable.h"
#include "LibTT/informative.h"

// How much clients can server process simultaniously
#ifndef CLIENTS_THREAD_POOL_CAPACITY
#   define CLIENTS_THREAD_POOL_CAPACITY 1000
#else
#   if CLIENTS_THREAD_POOL_CAPACITY < 0
#       error "Invalid value set for CLIENTS_THREAD_POOL_CAPACITY"
#   endif
#endif

namespace Ozzy::Base
{
    using boost::asio::ip::udp;

    class UdpServerBase : public Component::Configurable, public Component::Informative
    {
    protected:
        UdpServerBase(boost::asio::io_context &io_context, const std::string &config_path, const std::string &logger_name, const std::uint64_t doubles_count)
            : Informative(logger_name), m_general_socket(io_context), m_io_context(io_context), m_doubles_count(doubles_count)
        {
            // Initialize RNG
            std::random_device m_random_device;
            m_random_engine = std::mt19937_64(m_random_device());

            // Load configuration file
            if (!config_load(config_path))
            {
                LibLog::log_print(m_logger_name, "Failed starting server(error loading config file)");
                return;
            }

            if (!config_contains({"port", "ip_address"}))
            {
                LibLog::log_print(m_logger_name, "Failed starting server(error parsing config file)");
                return;
            }

            // Setup main server socket
            try
            {
                m_general_socket.open(udp::v4());

                const int port_number = std::stoi(m_config["port"]);
                m_general_socket.bind(udp::endpoint(udp::v4(), static_cast<std::uint16_t>(port_number)));
            }
            catch (const std::invalid_argument &)
            {
                LibLog::log_print(m_logger_name,
                                  "Port " + m_config["port"] + " cannot be casted to the string(invalid argument)");
                return;
            }
            catch (const std::out_of_range &)
            {
                LibLog::log_print(m_logger_name,
                                  "Port " + m_config["port"] + " cannot be casted from the string(out of range)");
                return;
            }
            catch (const std::exception &ex)
            {
                LibLog::log_print(m_logger_name, "Exception: " + std::string(ex.what()));
                return;
            }

            m_process_next_request.store(true, std::memory_order_release);
        }

        virtual ~UdpServerBase();

    public:
        // Start the server thread
        void start();

    private:
        // Receive the message from the client
        void start_receiving();

    protected:
        // Handle the received message
        virtual void handle_message(udp::endpoint client_endpoint, std::size_t bytes_received) noexcept = 0;

        // Handle the handshake between the server and the client
        virtual void handle_handshake(std::shared_ptr<LibUDP::Session>&& session) noexcept = 0;

        // Send individual frame to the client
        virtual bool send_frame(std::shared_ptr<LibUDP::Session>& session, Proto::Frame frame) noexcept = 0;

        // Send array of frames with random doubles from -x to x
        virtual bool send_frame_array(std::shared_ptr<LibUDP::Session>& session, double x) noexcept = 0;

    private:
        std::atomic<bool> m_should_quit;
        std::thread       m_server_thread;
        udp::socket       m_general_socket;
        std::thread       m_thread_cleaner;

    protected:
        mutable             std::uint64_t   m_doubles_count;
        static thread_local std::mt19937_64 m_random_engine;

        std::vector<std::shared_ptr<LibUDP::Session>> m_client_connections;
        std::vector<std::thread    >                  m_client_threads;

        std::atomic<bool>            m_process_next_request;
        boost::asio::io_context     &m_io_context;
        std::array<std::uint8_t, Proto::Constant::TransmittionUnitSize> m_receive_buffer{};
    };
}

#endif // __OZZY_UDP_SERVER_BASE__
