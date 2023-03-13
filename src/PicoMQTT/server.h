#pragma once

#include <list>

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "connection.h"
#include "publisher.h"
#include "subscriber.h"


namespace PicoMQTT {

class Server: public Publisher {
    public:
        class Client: public Connection, public Subscriber {
            public:
                Client(Server & server, const WiFiClient & client);
                Client(const Client &);

                void on_message(const char * topic, IncomingPacket & packet) override;

                Print & get_print() { return client; }
                const char * get_client_id() const { return client_id.c_str(); }

                virtual void loop() override;

            protected:
                Server & server;
                WiFiClient wifi_client;

                String client_id;

                virtual void on_subscribe(IncomingPacket & packet);
                virtual void on_unsubscribe(IncomingPacket & packet);

                virtual void handle_packet(IncomingPacket & packet) override;
        };

        Server(uint16_t port = 1883, size_t buffer_size = 1024, unsigned long keep_alive_tolerance_seconds = 10,
               unsigned long socket_timeout_seconds = 5);

        void loop();

        void begin();
        void stop();

        virtual Publish publish(const char * topic, const size_t payload_size,
                                uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) override;

        void publish(const char * topic, IncomingPacket & packet);

        virtual Connection::ConnectReturnCode auth(const char * client_id, const char * username, const char * password) { return Connection::CRC_ACCEPTED; }

        virtual void on_connect(const char * client_id) {}
        virtual void on_disconnect(const char * client_id) {}

        virtual void on_subscribe(const char * client_id, const char * topic) {}
        virtual void on_unsubscribe(const char * client_id, const char * topic) {}

    protected:
        virtual PrintMux get_subscribed(const char * topic);

        WiFiServer server;
        Buffer buffer;
        std::list<Client> clients;

        const unsigned long keep_alive_tolerance_seconds;
        const unsigned long socket_timeout_seconds;

};

}
