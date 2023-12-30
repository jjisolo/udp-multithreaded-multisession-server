#include "udp_server_base.h"
#include "LibLog/logging.h"
#include "LibUDP/networking.h"

namespace Ozzy::Base
{
    thread_local std::mt19937_64 UdpServerBase::m_random_engine;

    UdpServerBase::~UdpServerBase()
    {
        m_general_socket.close();
        stop();
    }

    void UdpServerBase::start_receiving()
    {
        while (!m_should_quit.load())
        {
            while (!m_process_next_request.load())
            {
            }

            udp::endpoint client_endpoint;
            boost::system::error_code error;

            const std::size_t bytes_received = m_general_socket.receive_from(
                boost::asio::buffer(m_receive_buffer),
                client_endpoint, 0, error
            );

            if (!error)
            {
                handle_message(client_endpoint, bytes_received);
            }
            else
            {
                LibLog::log_print(m_logger_name, "Receive error: " + error.message());
            }
        }
    }

    void UdpServerBase::start()
    {
        m_server_thread = std::thread([this]
        {
            start_receiving();
        });

        m_thread_cleaner = std::thread([this]
        {
            while (true)
            {
                std::size_t joined_total = 0;
                for (std::size_t i = 0; i < m_client_threads.size(); ++ i)
                {
                    if (m_client_threads[i].joinable())
                    {
                        m_client_threads[i].join();
                        m_client_connections[i]->close();
                        ++joined_total;
                    }
                }

                if (joined_total > 0)
                {
                    LibLog::log_print("[Ozzy::UdpServerBase::ThreadCleaner] ",
                                      "Released " + std::to_string(joined_total) + " finished threads");
                }
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
    }

    void UdpServerBase::stop()
    {
        m_server_thread.join();
        m_thread_cleaner.join();
    }
}
