#pragma once

#include <Arduino.h>

#include "connection.h"
#include "publisher.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"
#include "subscriber.h"

namespace PicoMQTT {

class BasicClient: public Connection, public Publisher {
    public:
        BasicClient(unsigned long keep_alive_seconds = 60, unsigned long socket_timeout_seconds = 10);

        BasicClient(const ::WiFiClient & client, unsigned long keep_alive_seconds = 60,
                    unsigned long socket_timeout_seconds = 10);

        bool connect(
            const char * host, uint16_t port = 1883,
            const char * id = "", const char * user = nullptr, const char * pass = nullptr,
            const char * willTopic = nullptr, const char * will_message = nullptr,
            const size_t will_message_length = 0, uint8_t willQos = 0, bool willRetain = false,
            const bool cleanSession = true,
            ConnectReturnCode * connect_return_code = nullptr);

        using Publisher::publish;
        virtual Publish publish(const char * topic, const size_t payload_size,
                                uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) override;

        bool subscribe(const String & topic, uint8_t qos = 0, uint8_t * qos_granted = nullptr);
        bool unsubscribe(const String & topic);

        void loop();

        virtual void on_connected() {}

    private:
        virtual bool on_publish_complete(const Publish & publish) override;
};

class Client: public BasicClient, public SubscribedMessageListener {
    public:
        Client(const char * host = nullptr, uint16_t port = 1883, const char * id = nullptr, const char * user = nullptr,
               const char * password = nullptr);

        using SubscribedMessageListener::subscribe;
        virtual SubscriptionId subscribe(const char * topic_filter, MessageCallback callback) override;
        virtual void unsubscribe(const char * topic_filter) override;

        virtual void loop();

        String host;
        uint16_t port;

        String client_id;
        String username;
        String password;

    protected:
        virtual void on_message(const char * topic, IncomingPacket & packet) override;
};

}
