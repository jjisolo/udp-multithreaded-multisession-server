#include "udp_client_v2.h"
#include "LibLog/logging.h"
#include "LibUDP/networking.h"
#include "LibFS/thread_cache_file.h"

namespace Ozzy::v2
{
    using boost::asio::ip::udp;

    bool UdpClient::send_handshake()
    {
        Proto::Handshake handshake;
        server_answer_t  answer;

        for (std::size_t attempts = 0; attempts < Proto::PacketRetransmitMaxAttempts; ++attempts)
        {
            if (!LibUDP::send_data(m_session, handshake))
            {
                LibLog::log_print(m_logger_name, "Unable to send data to the server");
                std::this_thread::sleep_for(std::chrono::milliseconds(Proto::Constant::PacketRetransmitWaitTimestamp));
                continue;
            }

            if (!LibUDP::receive_data(m_session, answer))
            {
                LibLog::log_print(m_logger_name, "Unable to receive answer from the server");
                continue;
            }

            switch (answer)
            {
                case Proto::v1::NACK:
                {

                    LibLog::log_print(m_logger_name, "[>Proto::v1] Internal server error when processing handshake");
                    return false;
                }

                case Proto::v2::CLIENT_THREAD_POOL_EXHAUSED:
                {
                    LibLog::log_print(m_logger_name, "[>Proto::v2] Server's client thread pool exhausted, handshake discarded");
                    return false;
                }

                case Proto::v1::DROP:
                {
                    LibLog::log_print(m_logger_name, "[>Proto::v1] Server cannot handle handshake, connection discarded");
                    return false;
                }

                case Proto::v1::ACK:
                {
                    LibLog::log_print(m_logger_name, "Server accepted handshake");
                    return true;
                }

                default:
                {
                    LibUDP::send_data(m_session, Proto::v1::DROP);
                    return false;
                }
            }
        }
    }

    bool UdpClient::validate_protocol_versions() noexcept
    {
        if (!LibUDP::send_data(m_session, Proto::VERSION_2))
        {
            LibLog::log_print(m_logger_name, "Unable to send protocol specification to the server");
            return false;
        }

        std::uint8_t answer;
        if (!LibUDP::receive_data(m_session, answer))
        {
            LibLog::log_print(m_logger_name, "Failed receiving data from the server");
            return false;
        }

        if (answer == Proto::v1::ERR_VERSIONS_INCOMPATIBLE)
        {
            server_answer_t server_protocol_version;

            if (!LibUDP::receive_data(m_session, server_protocol_version))
            {
                LibLog::log_print(m_logger_name, "Client and server versions are incompatible!");
                return false;
            }

            const std::string protocol_version_string = "[Proto::v" + std::to_string(server_protocol_version + 1) + " ] ";
            LibLog::log_print(m_logger_name, protocol_version_string + "Client and server versions are incompatible");
            return false;
        }

        return true;
    }

    void UdpClient::process_handshake()
    {
        // 1. Send handshake to the server, with the upper_bound payload,
        // which describes the maxium/minumum bound of the generated set with doubles.
        if (!send_handshake())
        {
            LibLog::log_print(m_logger_name, "Unable to send handshake to the server, connection discarded");
            return;
        }

        // 2. Server validates protocols versions, and answers with either
        //   -- ERR_VERSIONS_INCOMPATIBLE -> protocol versions are incompatible
        //   -- ACK                       -> protocols matching, everything good
        if (!validate_protocol_versions())
        {
            LibLog::log_print(m_logger_name, "Protocol versions validation failed");
            return;
        }
        LibLog::log_print(m_logger_name, "Validated server/client protocol versions");

        // 2.1 Wait 3 seconds and send double set upper bound for the generated
        // double values in the payload.
        std::this_thread::sleep_for(std::chrono::seconds(3));
        if (!LibUDP::send_data(m_session, m_upper_bound))
        {
            LibLog::log_print(m_logger_name, "Unable to send data to the server");
            LibUDP::send_data(m_session, Proto::v1::DROP);
            return;
        }

        // 3. Client receives the frames with the payload in them. Validates the checksum if the frame
        // and if all is correct, responds with Ack signal, otherwise with Nack, and re-receivers the frame.
        server_answer_t        answer;
        Proto::Frame           frame;
        LibFS::ThreadCacheFile cache_file;

        if (cache_file.initialized_sucessfully())
            LibUDP::send_data(m_session, Proto::v1::Answer::ACK);
        else
            LibUDP::send_data(m_session, Proto::v1::Answer::DROP);

        for (;;)
        {
            if (!LibUDP::receive_data(m_session, frame))
            {
                LibLog::log_print(m_logger_name, "Unable to receive the frame from the server");
                LibUDP::send_data(m_session, Proto::v1::Answer::NACK);
                continue;
            }

            // Validate the frame
            const std::uint64_t checksum = Proto::calculate_frame_checksum(frame);

            if (frame.checksum != checksum)
            {
                LibLog::log_print(m_logger_name, "Frame checksum calculation failed. Recieved frame data is corrupted");
                LibUDP::send_data(m_session, Proto::v1::Answer::NACK);
                continue;
            }

            for(unsigned ii = 0; ii < frame.length; ++ii)
                LibLog::log_print(m_logger_name, "Received payload: " + std::to_string(frame.payload[ii]));

            // Write to temporary cache file
            cache_file.write_frame(frame);

            // All good! We're now able to receive the next frame!
            LibUDP::send_data(m_session, Proto::v1::Answer::ACK);

            if (!LibUDP::receive_data(m_session, answer))
            {
                LibLog::log_print(m_logger_name, "Failed receiving data from the the client!");
                break;
            }

            if (answer != Proto::v1::Answer::CONT)
            {
                LibLog::log_print(m_logger_name, "Finished receiving the frame data from the server");
                break;
            }
        }

        cache_file.sort_file();
    }
}
