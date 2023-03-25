#pragma once

#include <Arduino.h>

namespace PicoMQTT {

class Packet {
    public:
        enum Type : uint8_t {
            ERROR = 0,
            CONNECT = 1 << 4,       // Client request to connect to Server
            CONNACK = 2 << 4,       // Connect Acknowledgment
            PUBLISH = 3 << 4,       // Publish message
            PUBACK = 4 << 4,        // Publish Acknowledgment
            PUBREC = 5 << 4,        // Publish Received (assured delivery part 1)
            PUBREL = 6 << 4,        // Publish Release (assured delivery part 2)
            PUBCOMP = 7 << 4,       // Publish Complete (assured delivery part 3)
            SUBSCRIBE = 8 << 4,     // Client Subscribe request
            SUBACK = 9 << 4,        // Subscribe Acknowledgment
            UNSUBSCRIBE = 10 << 4,  // Client Unsubscribe request
            UNSUBACK = 11 << 4,     // Unsubscribe Acknowledgment
            PINGREQ = 12 << 4,      // PING Request
            PINGRESP = 13 << 4,     // PING Response
            DISCONNECT = 14 << 4,   // Client is Disconnecting
        };

        Packet(uint8_t head, size_t size)
            : head(head), size(size), pos(0) {}

        Packet(Type type = ERROR, const uint8_t flags = 0, size_t size = 0)
            : Packet((uint8_t) type | (flags & 0xf), size) {
        }

        virtual ~Packet() {}

        Type get_type() const { return Type(head & 0xf0); }
        uint8_t get_flags() const { return head & 0x0f; }

        bool is_valid() { return get_type() != ERROR; }
        size_t get_remaining_size() const { return pos < size ? size - pos : 0; }

        const uint8_t head;
        const size_t size;

    protected:
        size_t pos;
};

}
