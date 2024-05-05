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
#include "utils.h"

namespace PicoMQTT {

class ServerSocketInterface {
    public:
        ServerSocketInterface() {}
        virtual ~ServerSocketInterface() {}

        ServerSocketInterface(const ServerSocketInterface &) = delete;
        const ServerSocketInterface & operator=(const ServerSocketInterface &) = delete;

        virtual void begin() = 0;
        virtual ::Client * accept_client() = 0;
};

template <typename Server>
class ServerSocket: public ServerSocketInterface, public Server {
    public:
        using Server::Server;

        virtual ::Client * accept_client() override {
            TRACE_FUNCTION
            auto client = Server::accept();
            if (!client) {
                // no connection
                return nullptr;
            }

            return new decltype(client)(client);
        };

        virtual void begin() override {
            TRACE_FUNCTION
            Server::begin();
        }
};

template <typename Server>
class ServerSocketProxy: public ServerSocketInterface {
    public:
        Server & server;

        ServerSocketProxy(Server & server): server(server) {}

        virtual ::Client * accept_client() override {
            TRACE_FUNCTION
            auto client = server.accept();
            if (!client) {
                // no connection
                return nullptr;
            }

            return new decltype(client)(client);
        };

        virtual void begin() override {
            TRACE_FUNCTION
            server.begin();
        }

};

class ServerSocketMux: public ServerSocketInterface {
    public:
        template <typename... Targs>
        ServerSocketMux(Targs & ... Fargs) {
            add(Fargs...);
        }

        virtual ::Client * accept_client() override {
            TRACE_FUNCTION
            for (auto & server : servers) {
                auto client = server->accept_client();
                if (client) {
                    // no connection
                    return client;
                }
            }
            return nullptr;
        };

        virtual void begin() override {
            TRACE_FUNCTION
            for (auto & server : servers) {
                server->begin();
            }
        }

    protected:
        template <typename Server>
        void add(Server & server) {
            servers.push_back(std::unique_ptr<ServerSocketInterface>(new ServerSocketProxy<Server>(server)));
        }

        template <typename Server, typename... Targs>
        void add(Server & server, Targs & ... Fargs) {
            add(server);
            add(Fargs...);
        }

        std::vector<std::unique_ptr<ServerSocketInterface>> servers;
};

class Server: public PicoMQTTInterface, public Publisher, public SubscribedMessageListener {
    public:
        class Client: public SocketOwner<std::unique_ptr<::Client>>, public Connection, public Subscriber {
            public:
                Client(Server & server, ::Client * client);

                void on_message(const char * topic, IncomingPacket & packet) override;

                Print & get_print() { return Connection::client; }
                const char * get_client_id() const { return client_id.c_str(); }

                virtual void loop() override;

                virtual const char * get_subscription_pattern(SubscriptionId id) const override;
                virtual SubscriptionId get_subscription(const char * topic) const override;
                virtual SubscriptionId subscribe(const String & topic_filter) override;
                virtual void unsubscribe(const String & topic_filter) override;

            protected:
                Server & server;
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

        Server(std::unique_ptr<ServerSocketInterface> socket);

        Server(uint16_t port = 1883)
            : Server(new ServerSocket<::WiFiServer>(port)) {
            TRACE_FUNCTION
        }

        template <typename ServerType>
        Server(ServerType & server)
            : Server(new ServerSocketProxy<ServerType>(server)) {
            TRACE_FUNCTION
        }

        template <typename ServerType, typename... Targs>
        Server(ServerType & server, Targs & ... Fargs): Server(new ServerSocketMux(server, Fargs...)) {
            TRACE_FUNCTION
        }

        void begin() override;
        void loop() override;

        using Publisher::begin_publish;
        virtual Publish begin_publish(const char * topic, const size_t payload_size,
                                      uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) override;

        unsigned long keep_alive_tolerance_millis;
        unsigned long socket_timeout_millis;

    protected:
        Server(ServerSocketInterface * socket)
            : Server(std::unique_ptr<ServerSocketInterface>(socket)) {
            TRACE_FUNCTION
        }

        virtual void on_message(const char * topic, IncomingPacket & packet);
        virtual ConnectReturnCode auth(const char * client_id, const char * username, const char * password) { return CRC_ACCEPTED; }

        virtual void on_connected(const char * client_id) {}
        virtual void on_disconnected(const char * client_id) {}

        virtual void on_subscribe(const char * client_id, const char * topic) {}
        virtual void on_unsubscribe(const char * client_id, const char * topic) {}

        virtual PrintMux get_subscribed(const char * topic);

        std::unique_ptr<ServerSocketInterface> server;
        std::list<std::unique_ptr<Client>> clients;
};

class ServerLocalSubscribe: public Server {
    public:
        using Server::Server;
        using Server::publish;
        using Server::publish_P;

        virtual bool publish(const char * topic, const void * payload, const size_t payload_size,
                             uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) override;

        virtual bool publish_P(const char * topic, PGM_P payload, const size_t payload_size,
                               uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) override;
};

}
