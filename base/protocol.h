#ifndef __OZZ_PROTOCOL__
#define __OZZ_PROTOCOL__

#include <cstdint>
namespace Ozzy::Proto
{
    enum Constant
    {
        // How much time we need to wait before re-sending the packet,
        // I thought that it's correct to place this value in the protocol
        // standard(e.g: be defined here).
        PacketRetransmitWaitTimestamp = 200,

        // How much block of data can be retrasnmitted until the connection
        // should be declared as unstable and dropped down.
        PacketRetransmitMaxAttempts   = 4,

        // How much we wait until the server responds with ... something
        PacketWaitTimeoutMillis       = 400,

        // Maximal transmittion unit size(bits)
        TransmittionUnitSize          = 1500 * 8,
    };

    enum Version
    {
        VERSION_1 = 0,
        VERSION_2,
    };

    // Specifies the message type, should be the first 8 bits of
    // every message in the protocol. We need that to differentiate
    // between for exanple handshake and frame.
    enum MessageType
    {
        MESSAGE_TYPE_FRAME = 0,
        MESSAGE_TYPE_HANDSHAKE,
    };

    constexpr std::size_t OZZY_PAYLOAD_COUNT_PER_CHUNK        = 175;
    constexpr std::size_t OZZY_MAXIMAL_TRANSMITTION_UNIT_SIZE = TransmittionUnitSize;

    // Assuming MTU is ~1500 bytes:
    //
    // (64bytes + (64bytes * PAYLOAD_COUNT_PER_CHUNK(175units))) = 1408 bytes are sended per frame.
    //
    // 92 bytes left to upper ~MTU bound.
    //
    // This idea with bitfields is because we want to keep constant offset to the payload.
    constexpr std::size_t FRAME_BITS_PER_TYPE     = 8;
    constexpr std::size_t FRAME_BITS_PER_LENGTH   = 8;
    constexpr std::size_t FRAME_BITS_PER_CHECKSUM = 48;
    static_assert((FRAME_BITS_PER_TYPE + FRAME_BITS_PER_LENGTH + FRAME_BITS_PER_CHECKSUM) / 8 == sizeof(std::uint64_t));

#pragma pack(push, 1)
    struct Frame
    {
        // 8 readonly bits for the type of this message
        const std::uint64_t type    : FRAME_BITS_PER_TYPE = MESSAGE_TYPE_FRAME;

        // The maximum length of the packet payload is 255 elements.
        // It is ((255 * 8) - 1500) = 540 bytes above the MTU chunk.
        std::uint64_t       length  : FRAME_BITS_PER_LENGTH;

        // Rest of the bits for the checksum of the payload below.
        std::uint64_t       checksum: FRAME_BITS_PER_CHECKSUM;

        // Payload with doubles, as requested >.<.
        double payload[OZZY_PAYLOAD_COUNT_PER_CHUNK];
    };
    static_assert(OZZY_PAYLOAD_COUNT_PER_CHUNK * sizeof(double) < OZZY_MAXIMAL_TRANSMITTION_UNIT_SIZE - sizeof(std::uint64_t)*8);
    static_assert(sizeof(Frame) <= OZZY_MAXIMAL_TRANSMITTION_UNIT_SIZE);
#pragma pack(pop)

#pragma pack(push, 1)
    struct Handshake
    {
        // 8 readonly bits for the type of this message
        const std::uint8_t type = MESSAGE_TYPE_HANDSHAKE;
    };
    static_assert(sizeof(Handshake) <= OZZY_MAXIMAL_TRANSMITTION_UNIT_SIZE);
#pragma pack(pop)

    namespace v1
    {
        enum Answer
        {
            ACK ,
            NACK,
            DROP,
            CONT,
            BRK ,
            ERR_VERSIONS_INCOMPATIBLE,

            LAST_OPCODE = 50
        };
    }

    namespace v2
    {
        enum Answer
        {
            CLIENT_THREAD_POOL_EXHAUSED = v1::LAST_OPCODE,

            LAST_OPCODE = 100,
        };
    }

    std::uint64_t calculate_frame_checksum(Frame frame);
};

#endif // __OZZ_PROTOCOL__
