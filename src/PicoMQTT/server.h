#pragma once

#include <list>
#include <set>

#include <Arduino.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#error "This board is not supported."
#endif

#include "debug.h"
#include "incoming_packet.h"
#include "connection.h"
#include "publisher.h"
#include "subscriber.h"
#include "pico_interface.h"

namespace PicoMQTT {

class BasicServer: public PicoMQTTInterface, public Publisher {
    public:
        class Client: public Connection, public Subscriber {
            public:
                Client(BasicServer & server, const WiFiClient & client);
                Client(const Client &);

                void on_message(const char * topic, IncomingPacket & packet) override;

                Print & get_print() { return client; }
                const char * get_client_id() const { return client_id.c_str(); }

                virtual void loop() override;

                virtual const char * get_subscription_pattern(SubscriptionId id) const override;
                virtual SubscriptionId get_subscription(const char * topic) const override;
                virtual SubscriptionId subscribe(const String & topic_filter) override;
                virtual void unsubscribe(const String & topic_filter) override;

            protected:
                BasicServer & server;
                String client_id;
                std::set<Subscription> subscriptions;

                virtual void on_subscribe(IncomingPacket & packet);
                virtual void on_unsubscribe(IncomingPacket & packet);

                virtual void handle_packet(IncomingPacket & packet) override;
        };

        class IncomingPublish: public IncomingPacket {
            public:
                IncomingPublish(IncomingPacket & packet, Publish & publish);
                IncomingPublish(const IncomingPublish &) = delete;
                ~IncomingPublish();

                virtual int read(uint8_t * buf, size_t size) override;
                virtual int read() override;

            protected:
                Publish & publish;
        };

        BasicServer(uint16_t port = 1883, unsigned long keep_alive_tolerance_seconds = 10,
                    unsigned long socket_timeout_seconds = 5);

        void begin() override;
        void stop() override;
        void loop() override;

        using Publisher::begin_publish;
        virtual Publish begin_publish(const char * topic, const size_t payload_size,
                                      uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) override;

    protected:
        virtual void on_message(const char * topic, IncomingPacket & packet);
        virtual ConnectReturnCode auth(const char * client_id, const char * username, const char * password) { return CRC_ACCEPTED; }

        virtual void on_connected(const char * client_id) {}
        virtual void on_disconnected(const char * client_id) {}

        virtual void on_subscribe(const char * client_id, const char * topic) {}
        virtual void on_unsubscribe(const char * client_id, const char * topic) {}

        virtual PrintMux get_subscribed(const char * topic);

        WiFiServer server;
        std::list<Client> clients;

        const unsigned long keep_alive_tolerance_seconds;
        const unsigned long socket_timeout_seconds;

};

class Server: public BasicServer, public SubscribedMessageListener {
    public:
        using BasicServer::BasicServer;
        virtual void on_message(const char * topic, IncomingPacket & packet) override;
};

}
