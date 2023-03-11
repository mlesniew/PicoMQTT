#pragma once

#include <functional>

#include <Arduino.h>
#include "Client.h"

#include "buffer.h"
#include "client_wrapper.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"

namespace NanoMQTT {

enum ConnectReturnCode : uint8_t {
    crc_connection_accepted = 0,
    crc_unacceptable_protocol_version = 1,
    crc_identifier_rejected = 2,
    crc_server_unavailable = 3,
    crc_bad_username_or_password = 4,
    crc_not_authorized = 5,

    // internal
    crc_undefined = 255,
};

class Client {
    public:
        class Publish: public OutgoingPacket {
            public:
                Publish(
                    Client & mqtt,
                    const char * topic, size_t topic_size,
                    size_t payload_size,
                    uint8_t qos, bool retain, bool dup);

                virtual bool send() override;

                const uint8_t qos;
                const uint16_t message_id;

            protected:
                Client & mqtt;
        };

        Client(::Client & client, size_t buffer_size = 512, uint16_t keep_alive = 60000);
        virtual ~Client() {}

        bool connect(
            const char * host, uint16_t port,
            const char * id, const char * user = nullptr, const char * pass = nullptr,
            const char * willTopic = nullptr, const char * will_message = nullptr,
            const size_t will_message_length = 0, uint8_t willQos = 0, bool willRetain = false,
            const bool cleanSession = true,
            ConnectReturnCode * connect_return_code = nullptr);

        void disconnect();

        Publish publish(const char * topic, const size_t payload_size,
                        uint8_t qos = 0, bool retain = false, bool dup = false);

        bool publish(const char * topic, const void * payload, const size_t payload_size,
                     uint8_t qos = 0, bool retain = false, bool dup = false);

        bool publish(const char * topic, const char * payload,
                     uint8_t qos = 0, bool retain = false, bool dup = false);

        bool publish(const String & topic, const String & payload,
                     uint8_t qos = 0, bool retain = false, bool dup = false);

        bool publish_P(const char * topic, const void * payload, const size_t payload_size,
                       uint8_t qos = 0, bool retain = false, bool dup = false);

        bool publish_P(const char * topic, const char * payload,
                       uint8_t qos = 0, bool retain = false, bool dup = false);

        bool subscribe(const String & topic, uint8_t qos = 0, uint8_t * qos_granted = nullptr);
        bool unsubscribe(const String & topic);

        void loop();
        bool connected();

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
        };

        OutgoingPacket build_packet(Packet::Type type, uint8_t flags, size_t length);
        bool send_simple_packet(Packet::Type type, uint8_t flags = 0);

        void wait_for_reply(Packet::Type type, std::function<void(IncomingPacket & packet)> handler);

        virtual void on_message_too_big(IncomingPacket & packet);
        virtual void on_message(const char * topic, const char * payload, size_t payload_size);
        virtual void on_message(const char * topic, IncomingPacket & packet);

        virtual void on_timeout();
        virtual void on_protocol_violation();
        virtual void on_disconnect();

        ClientWrapper client;
        Buffer buffer;
        MessageIdGenerator messageIdGenerator;
        uint16_t keep_alive_millis;

    private:
        void handle_packet(IncomingPacket & packet);
        bool confirm_publish(const Publish & publish);
};

}
