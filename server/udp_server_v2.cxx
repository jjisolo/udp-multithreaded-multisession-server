#include "udp_server_v2.h"
#include "LibUDP/networking.h"
#include "LibLog/logging.h"

namespace Ozzy::v2
{
    bool UdpServer::send_frame(udp::endpoint &client_endpoint, udp::socket &client_socket,
                               Proto::Frame frame) const noexcept
    {
        client_answer_t answer;

        for (std::size_t i = 0u; i < Proto::Constant::PacketRetransmitMaxAttempts; ++i)
        {
            if (!LibUDP::send_data(client_endpoint, client_socket, frame))
            {
                LibLog::log_print(m_logger_name,
                                  "Failed sending frame(sending failed) to the client, connection unstable");
                return false;
            }

            if (!LibUDP::receive_data(client_endpoint, client_socket, answer))
            {
                LibLog::log_print(m_logger_name,
                                  "Failed sending frame(receiving answer failed) to the client, connection unstable");
                return false;
            }

            // If client answered with Ack, it means that it received frame succesfully, otherwise try sending the
            // frame again.
            if (answer == Proto::v1::Answer::ACK)
            {
                return true;
            }
            if (answer == Proto::v1::Answer::DROP)
            {
                LibLog::log_print(m_logger_name,
                                  "Client requested to drop the connection " + LibLog::serialize_endpoint(
                                      client_endpoint));
                return false;
            }

            LibLog::log_print(m_logger_name, "Failed sending frame to the client retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(Proto::Constant::PacketRetransmitWaitTimestamp));
        }

        return false;
    }

    bool UdpServer::send_frame_array(udp::endpoint &client_endpoint, udp::socket &client_socket,
                                     double x) const noexcept
    {
        const int unsigned frames_total = (m_doubles_count + Proto::OZZY_PAYLOAD_COUNT_PER_CHUNK - 1) /
                                          Proto::OZZY_PAYLOAD_COUNT_PER_CHUNK;
        int unsigned doubles_remain = m_doubles_count;

        // Setup initial frame data
        Proto::Frame frame;

        // Setup initial RNG data
        std::random_device seed = std::random_device();
        m_random_engine = std::mt19937_64(seed());

        std::uniform_real_distribution<double> real_distribution(-x, x);

        for (std::size_t k = 0; k < frames_total; ++k)
        {
            // Bake the frame
            const std::size_t remaining = doubles_remain % Proto::OZZY_PAYLOAD_COUNT_PER_CHUNK;
            frame.length = remaining == 0 ? Proto::OZZY_PAYLOAD_COUNT_PER_CHUNK : remaining;

            for (std::size_t i = 0; i < frame.length; ++i)
            {
                frame.payload[i] = real_distribution(m_random_engine);
            }

            frame.checksum = Proto::calculate_frame_checksum(frame);

            // Try send frame to the client, until it will respond with the
            // ACK answer, meaning the recieve does not need to be re-sended.
            //
            // If the client repsponded with Nack more than PacketRetransmitMaxAttempt, than
            // connection is marked as unstable and should be closed.
            if (!send_frame(client_endpoint, client_socket, frame))
            {
                LibLog::log_print(m_logger_name, "Unable to send the frame to the client");
                return false;
            }

            doubles_remain -= frame.length;

            if (k + 1 == frames_total)
                LibUDP::send_data(client_endpoint, client_socket, Proto::v1::Answer::BRK);
            else
                LibUDP::send_data(client_endpoint, client_socket, Proto::v1::Answer::CONT);
        }

        return true;
    }

    void UdpServer::handle_message(udp::endpoint client_endpoint, const std::size_t bytes_received) noexcept
    {
        m_process_next_request.store(false);

        // Create the socket, that will handle the response routine for this message
        boost::asio::io_context io_context;
        udp::socket client_socket(io_context);
        client_socket.open(udp::v4());
        client_socket.bind(udp::endpoint(udp::v4(), 0));

        // Validate that we received at least enough data to validate the
        // protocol version that client uses
        if (bytes_received < sizeof(std::uint8_t))
        {
            LibLog::log_print(m_logger_name, "Recieve failed, requested re-transmit");
            LibUDP::send_data(client_endpoint, client_socket, Proto::v1::NACK);
        }

        const std::uint8_t message = m_receive_buffer[0];
        if (message == Proto::MESSAGE_TYPE_HANDSHAKE)
        {
            if (bytes_received != sizeof(Proto::Handshake))
            {
                LibLog::log_print(m_logger_name,
                                  "Error recieve data does not match expected struct(Ozzy::Proto::Handshake)");
                LibUDP::send_data(client_endpoint, client_socket, Proto::v1::NACK);
            }

            // Create the separate thread for the client
            if (m_client_threads.size() < CLIENTS_THREAD_POOL_CAPACITY)
            {
                m_client_threads.emplace_back(&UdpServer::handle_handshake, this, client_endpoint,
                                              std::move(client_socket));
            }
            else
            {
                LibLog::log_print(m_logger_name, "Too many threads for the client requests, handshake dropped");
                LibUDP::send_data(client_endpoint, client_socket, Proto::v2::CLIENT_THREAD_POOL_EXHAUSED);
                client_socket.close();
            }
        }
        else
        {
            LibLog::log_print(m_logger_name,
                              "Client " + LibLog::serialize_endpoint(client_endpoint) +
                              " did not start the transmit operation with the hadnshake. Connection discarded.");
            LibUDP::send_data(client_endpoint, client_socket, Proto::v1::DROP);
            client_socket.close();
        }
    }

    void UdpServer::handle_handshake(udp::endpoint client_endpoint, udp::socket &&client_socket) noexcept
    {
        // 1. Recieve handshake from the client, answer with Ack, meaning that handhsake data
        // transmitted with no errors
        LibLog::log_print(m_logger_name, "Recieved handshake from " + LibLog::serialize_endpoint(client_endpoint));
        LibUDP::send_data(client_endpoint, client_socket, Proto::v1::ACK);

        // Validate client's version
        std::uint8_t client_version;

        if (!LibUDP::receive_data(client_endpoint, client_socket, client_version))
        {
            if (client_version < Proto::VERSION_2)
            {
                LibUDP::send_data(client_endpoint, client_socket, Proto::v1::ERR_VERSIONS_INCOMPATIBLE);
                LibUDP::send_data(client_endpoint, client_socket, Proto::VERSION_2);
                client_socket.close();
                return;
            }
        }
        LibUDP::send_data(client_endpoint, client_socket, Proto::v1::ACK);

#if 0
        // Handshake is unused here, maybe add some values later
        Proto::Handshake handshake;
        std::memcpy(&handshake, m_receive_buffer.data(), sizeof(Proto::Handshake));
#endif
        m_process_next_request.store(true);

        // 2. Get the X upper_bound from the client
        double x_upper_bound;

        if (!LibUDP::receive_data(client_endpoint, client_socket, x_upper_bound))
        {
            LibLog::log_print(m_logger_name, "Unable to receive the answer from the client, handshake failed");
            return;
        }

        // Receive answer from the client, if it's Drop, then close the session.
        std::uint8_t client_answer;

        if (!LibUDP::receive_data(client_endpoint, client_socket, client_answer))
        {
            LibLog::log_print(m_logger_name, "Unable to receive the answer from the client, handshake failed");
            return;
        }

        if (client_answer == Proto::v1::Answer::DROP)
        {
            LibLog::log_print(m_logger_name, "Client requested to close the session");
            return;
        }

        LibLog::log_print(m_logger_name, "Succesfully handshaked with " + LibLog::serialize_endpoint(client_endpoint));

        // 3. Start sending the packets to the client. According to the MTU of ~1500.
        // Meaninng that each packet should be less than 1500 bytes.
        //
        // Validation of the packet is performed using checksum calculation
        // of the payload.
        // We !do not! track the missing packets, it's the RTMP/TCP style.
        LibLog::log_print(m_logger_name, "Start sending frames to " + LibLog::serialize_endpoint(client_endpoint));

        if (!send_frame_array(client_endpoint, client_socket, x_upper_bound))
        {
            LibLog::log_print(m_logger_name,
                              "Discarded connection with " + LibLog::serialize_endpoint(client_endpoint));
            LibUDP::send_data(client_endpoint, client_socket, Proto::v1::Answer::DROP);
            return;
        }

        LibLog::log_print(m_logger_name,
                          "Sucessfully sended frames to the client " + LibLog::serialize_endpoint(client_endpoint));

        client_socket.close();
        LibLog::log_print(m_logger_name,
                          "Closed the connection with " + LibLog::serialize_endpoint(client_endpoint));
    }
}
