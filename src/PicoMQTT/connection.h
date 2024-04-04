#pragma once

#include <functional>

#include <Arduino.h>

#include "client_wrapper.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"

namespace PicoMQTT {

enum ConnectReturnCode : uint8_t {
    CRC_ACCEPTED = 0,
    CRC_UNACCEPTABLE_PROTOCOL_VERSION = 1,
    CRC_IDENTIFIER_REJECTED = 2,
    CRC_SERVER_UNAVAILABLE = 3,
    CRC_BAD_USERNAME_OR_PASSWORD = 4,
    CRC_NOT_AUTHORIZED = 5,

    // internal
    CRC_UNDEFINED = 255,
};

class Connection {
    public:
        Connection(unsigned long keep_alive_seconds = 0, unsigned long socket_timeout_seconds = 15);
        Connection(const ::WiFiClient & client, unsigned long keep_alive_seconds = 0,
                   unsigned long socket_timeout_seconds = 15);
        Connection(const Connection &) = default;

        virtual ~Connection() {}

        bool connected();
        void disconnect();

        virtual void loop();

    protected:
        class MessageIdGenerator {
            public:
                MessageIdGenerator(): value(0) {}
                uint16_t generate() {
                    if (++value == 0) { value = 1; }
                    return value;
                }

                void reset() { value = 0; }

            protected:
                uint16_t value;
        } message_id_generator;

        OutgoingPacket build_packet(Packet::Type type, uint8_t flags = 0, size_t length = 0);

        void wait_for_reply(Packet::Type type, std::function<void(IncomingPacket & packet)> handler);

        virtual void on_topic_too_long(const IncomingPacket & packet) {}
        virtual void on_message(const char * topic, IncomingPacket & packet) {}

        virtual void on_timeout();
        virtual void on_protocol_violation();
        virtual void on_disconnect();

        ClientWrapper client;
        unsigned long keep_alive_millis;

        virtual void handle_packet(IncomingPacket & packet);

    protected:
        unsigned long get_millis_since_last_read() const;
        unsigned long get_millis_since_last_write() const;

    private:
        unsigned long last_read;
        unsigned long last_write;
        void send_ack(Packet::Type ack_type, uint16_t msg_id);
};

}
