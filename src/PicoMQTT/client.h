#pragma once

#include <Arduino.h>

#include "connection.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"
#include "pico_interface.h"
#include "publisher.h"
#include "subscriber.h"
#include "utils.h"

namespace PicoMQTT {

class BasicClient: public PicoMQTTInterface, public Connection, public Publisher {
    public:
        BasicClient(::Client & client, unsigned long keep_alive_millis = 60 * 1000,
                    unsigned long socket_timeout_millis = 10 * 1000);

        bool connect(
            const char * host, uint16_t port = 1883,
            const char * id = "", const char * user = nullptr, const char * pass = nullptr,
            const char * will_topic = nullptr, const char * will_message = nullptr,
            const size_t will_message_length = 0, uint8_t willQos = 0, bool willRetain = false,
            const bool cleanSession = true,
            ConnectReturnCode * connect_return_code = nullptr);

        using Publisher::begin_publish;
        virtual Publish begin_publish(const char * topic, const size_t payload_size,
                                      uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) override;

        bool subscribe(const String & topic, uint8_t qos = 0, uint8_t * qos_granted = nullptr);
        bool unsubscribe(const String & topic);

        void loop() override;

        virtual void on_connect() {}

    private:
        virtual bool on_publish_complete(const Publish & publish) override;
};

class ClientSocketInterface {
    public:
        virtual ::Client & get_client() = 0;
        virtual ~ClientSocketInterface() {}
};

class ClientSocketProxy: public ClientSocketInterface {
    public:
        ClientSocketProxy(::Client & client): client(client) {}
        virtual ::Client & get_client() override { return client; }
        ::Client & client;
};

template <typename ClientType>
class ClientSocket: public ClientType, public ClientSocketInterface {
    public:
        using ClientType::ClientType;
        virtual ::Client & get_client() override { return *this; }
};

class Client: public SocketOwner<std::unique_ptr<ClientSocketInterface>>, public BasicClient,
            public SubscribedMessageListener {
    public:
        Client(const char * host = nullptr, uint16_t port = 1883, const char * id = nullptr, const char * user = nullptr,
               const char * password = nullptr, unsigned long reconnect_interval_millis = 5 * 1000,
               unsigned long keep_alive_millis = 60 * 1000, unsigned long socket_timeout_millis = 10 * 1000)
            : Client(new ClientSocket<::WiFiClient>(), host, port, id, user, password, reconnect_interval_millis, keep_alive_millis,
                     socket_timeout_millis) {
        }

        template <typename ClientType>
        Client(ClientType & client, const char * host = nullptr, uint16_t port = 1883, const char * id = nullptr,
               const char * user = nullptr, const char * password = nullptr,
               unsigned long reconnect_interval_millis = 5 * 1000,
               unsigned long keep_alive_millis = 60 * 1000, unsigned long socket_timeout_millis = 10 * 1000)
            : Client(new ClientSocketProxy(client), host, port, id, user, password, reconnect_interval_millis, keep_alive_millis,
                     socket_timeout_millis) {
        }

        using SubscribedMessageListener::subscribe;
        virtual SubscriptionId subscribe(const String & topic_filter, MessageCallback callback) override;
        virtual void unsubscribe(const String & topic_filter) override;

        virtual void loop() override;

        String host;
        uint16_t port;

        String client_id;
        String username;
        String password;

        struct {
            String topic;
            String payload;
            uint8_t qos;
            bool retain;
        } will;

        unsigned long reconnect_interval_millis;

        std::function<void()> connected_callback;
        std::function<void()> disconnected_callback;
        std::function<void()> connection_failure_callback;

        virtual void on_connect() override;
        virtual void on_disconnect() override;

    protected:
        Client(ClientSocketInterface * client,
               const char * host, uint16_t port, const char * id, const char * user, const char * password,
               unsigned long reconnect_interval_millis, unsigned long keep_alive_millis, unsigned long socket_timeout_millis);

        unsigned long last_reconnect_attempt;
        virtual void on_message(const char * topic, IncomingPacket & packet) override;

};

}
