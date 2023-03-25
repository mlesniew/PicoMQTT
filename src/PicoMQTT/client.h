#pragma once

#include <Arduino.h>

#include "connection.h"
#include "publisher.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"
#include "message_listener.h"

namespace PicoMQTT {

class Client: public Connection, public Publisher, public MessageListener {
    public:

        Client(::Client & client, size_t buffer_size = 512, unsigned long keep_alive_seconds = 60,
               unsigned long socket_timeout_seconds = 10);

        bool connect(
            const char * host, uint16_t port,
            const char * id, const char * user = nullptr, const char * pass = nullptr,
            const char * willTopic = nullptr, const char * will_message = nullptr,
            const size_t will_message_length = 0, uint8_t willQos = 0, bool willRetain = false,
            const bool cleanSession = true,
            ConnectReturnCode * connect_return_code = nullptr);

        virtual Publish publish(const char * topic, const size_t payload_size,
                                uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) override;

        bool subscribe(const String & topic, uint8_t qos = 0, uint8_t * qos_granted = nullptr);
        bool unsubscribe(const String & topic);

        void loop();

    protected:
        virtual void on_message(const char * topic, const char * payload, size_t payload_size);
        virtual void on_message(const char * topic, IncomingPacket & packet);

        Buffer buffer_instance;

    private:
        virtual bool on_publish_complete(const Publish & publish) override;
};

}
