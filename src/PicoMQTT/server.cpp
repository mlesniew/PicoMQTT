#include "server.h"

#include "config.h"
#include "debug.h"

namespace {

class BufferClient : public ::Client {
public:
    BufferClient(const void * ptr) : ptr((const char *)ptr) { TRACE_FUNCTION; }

    // these methods are nop dummies
    virtual int connect(IPAddress ip, uint16_t port) override final {
        TRACE_FUNCTION;
        return 0;
    }
    virtual int connect(const char * host, uint16_t port) override final {
        TRACE_FUNCTION;
        return 0;
    }
#ifdef PICOMQTT_EXTRA_CONNECT_METHODS
    virtual int connect(IPAddress ip, uint16_t port,
                        int32_t timeout) override final {
        TRACE_FUNCTION;
        return 0;
    }
    virtual int connect(const char * host, uint16_t port,
                        int32_t timeout) override final {
        TRACE_FUNCTION;
        return 0;
    }
#endif
    virtual size_t write(const uint8_t * buffer, size_t size) override final {
        TRACE_FUNCTION;
        return 0;
    }
    virtual size_t write(uint8_t value) override final {
        TRACE_FUNCTION;
        return 0;
    }
    virtual void flush() override final { TRACE_FUNCTION; }
    virtual void stop() override final { TRACE_FUNCTION; }

    // these methods are in jasager mode
    virtual int available() override final {
        TRACE_FUNCTION;
        return std::numeric_limits<int>::max();
    }
    virtual operator bool() override final {
        TRACE_FUNCTION;
        return true;
    }
    virtual uint8_t connected() override final {
        TRACE_FUNCTION;
        return true;
    }

    // actual reads implemented here
    virtual int read(uint8_t * buf, size_t size) override {
        memcpy(buf, ptr, size);
        ptr += size;
        return size;
    }

    virtual int read() override final {
        TRACE_FUNCTION;
        uint8_t ret;
        read(&ret, 1);
        return ret;
    }

    virtual int peek() override final {
        TRACE_FUNCTION;
        const int ret = read();
        --ptr;
        return ret;
    }

protected:
    const char * ptr;
};

class BufferClientP : public BufferClient {
public:
    using BufferClient::BufferClient;

    virtual int read(uint8_t * buf, size_t size) override {
        memcpy_P(buf, ptr, size);
        ptr += size;
        return size;
    }
};

}  // namespace

namespace PicoMQTT {

Server::PrintMux::PrintMux(Server & server) : server(server) {}

size_t Server::PrintMux::write(uint8_t c) {
    TRACE_FUNCTION;
    for (Client * client = server.clients; client; client = client->next) {
        if (client->subscribed) {
            client->get_print().write(c);
        }
    }
    return 1;
}

size_t Server::PrintMux::write(const uint8_t * buf, size_t size) {
    TRACE_FUNCTION;
    for (Client * client = server.clients; client; client = client->next) {
        if (client->subscribed) {
            client->get_print().write(buf, size);
        }
    }
    return size;
}

void Server::PrintMux::flush() {
    TRACE_FUNCTION;
    for (Client * client = server.clients; client; client = client->next) {
        if (client->subscribed) {
            client->get_print().flush();
        }
    }
}

Server::Client::Client(Server & server, ::Client * client)
    : SocketOwner(client),
      Connection(*socket, 0, server.socket_timeout_millis),
      next(nullptr),
      subscribed(false),
      server(server),
      client_id("<unknown>") {
    TRACE_FUNCTION;
    wait_for_reply(Packet::CONNECT, [this](IncomingPacket & packet) {
        TRACE_FUNCTION;

        auto connack = [this](ConnectReturnCode crc) {
            TRACE_FUNCTION;
            auto connack = build_packet(Packet::CONNACK, 0, 2);
            connack.write_u8(0); /* session present always set to zero */
            connack.write_u8(crc);
            connack.send();
            if (crc != CRC_ACCEPTED) {
                Connection::client.stop();
            }
        };

        {
            // MQTT protocol identifier
            char buf[4];

            if (packet.read_u16() != 4) {
                on_protocol_violation();
                return;
            }

            packet.read((uint8_t *)buf, 4);

            if (memcmp(buf, "MQTT", 4) != 0) {
                on_protocol_violation();
                return;
            }
        }

        const uint8_t protocol_level = packet.read_u8();
        if (protocol_level != 4) {
            on_protocol_violation();
            return;
        }

        const uint8_t connect_flags = packet.read_u8();
        const bool has_user = connect_flags & (1 << 7);
        const bool has_pass = connect_flags & (1 << 6);
        const bool will_retain = connect_flags & (1 << 5);
        const uint8_t will_qos = (connect_flags >> 3) & 0b11;
        const bool has_will = connect_flags & (1 << 2);
        /* const bool clean_session = connect_flags & (1 << 1); */

        if ((has_pass && !has_user) || (will_qos > 2) ||
            (!has_will && ((will_qos > 0) || will_retain))) {
            on_protocol_violation();
            return;
        }

        const unsigned long keep_alive_seconds = packet.read_u16();
        keep_alive_millis = keep_alive_seconds
                                ? (keep_alive_seconds * 1000 +
                                   this->server.keep_alive_tolerance_millis)
                                : 0;

        {
            const size_t client_id_size = packet.read_u16();
            if (client_id_size > PICOMQTT_MAX_CLIENT_ID_SIZE) {
                connack(CRC_IDENTIFIER_REJECTED);
                return;
            }

            char client_id_buffer[client_id_size + 1];
            packet.read_string(client_id_buffer, client_id_size);
            client_id = client_id_buffer;
        }

        if (client_id.isEmpty()) {
            client_id = String((unsigned int)(this), HEX);
        }

        if (has_will) {
            packet.ignore(packet.read_u16());  // will topic
            packet.ignore(packet.read_u16());  // will payload
        }

        // read username
        const size_t user_size = has_user ? packet.read_u16() : 0;
        if (user_size > PICOMQTT_MAX_USERPASS_SIZE) {
            connack(CRC_BAD_USERNAME_OR_PASSWORD);
            return;
        }
        char user[user_size + 1];
        if (user_size && !packet.read_string(user, user_size)) {
            on_timeout();
            return;
        }

        // read password
        const size_t pass_size = has_pass ? packet.read_u16() : 0;
        if (pass_size > PICOMQTT_MAX_USERPASS_SIZE) {
            connack(CRC_BAD_USERNAME_OR_PASSWORD);
            return;
        }
        char pass[pass_size + 1];
        if (pass_size && !packet.read_string(pass, pass_size)) {
            on_timeout();
            return;
        }

        const auto connect_return_code =
            this->server.auth(client_id.c_str(), has_user ? user : nullptr,
                              has_pass ? pass : nullptr);

        connack(connect_return_code);
    });
}

void Server::Client::on_message(const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION;

    const size_t payload_size = packet.get_remaining_size();
    auto publish = server.begin_publish(topic, payload_size);

    // Always notify the server about the message
    {
        IncomingPublish incoming_publish(packet, publish);
        server.on_message(topic, incoming_publish);
    }

    publish.send();
}

void Server::Client::on_subscribe(IncomingPacket & subscribe) {
    TRACE_FUNCTION;
    const uint16_t message_id = subscribe.read_u16();

    if ((subscribe.get_flags() != 0b0010) || !message_id) {
        on_protocol_violation();
        return;
    }

    uint8_t suback_codes[(PICOMQTT_MAX_SUBSCRIPTIONS_PER_PACKET + 7) / 8] = {};
    size_t suback_codes_count = 0;

    for (; subscribe.get_remaining_size(); ++suback_codes_count) {
        const size_t topic_size = subscribe.read_u16();

        if (suback_codes_count >= PICOMQTT_MAX_SUBSCRIPTIONS_PER_PACKET) {
            // Too many subscriptions requested in one packet.
            // Report subscription failure in SUBACK for the remaining ones.
            subscribe.ignore(topic_size);
            subscribe.read_u8();  // qos
            continue;
        } else if (topic_size > PICOMQTT_MAX_TOPIC_SIZE) {
            subscribe.ignore(topic_size);
            subscribe.read_u8();
            suback_codes[suback_codes_count >> 3] |=
                1 << (suback_codes_count & 7);
        } else {
            char topic[topic_size + 1];
            if (!subscribe.read_string(topic, topic_size)) {
                // connection error
                return;
            }
            uint8_t qos = subscribe.read_u8();
            if (qos > 2) {
                on_protocol_violation();
                return;
            }
            if (this->subscribe(topic)) {
                server.on_subscribe(client_id.c_str(), topic);
            } else {
                suback_codes[suback_codes_count >> 3] |=
                    1 << (suback_codes_count & 7);
            }
        }
    }

    auto suback = build_packet(Packet::SUBACK, 0, 2 + suback_codes_count);
    suback.write_u16(message_id);
    for (size_t i = 0; i < suback_codes_count; ++i) {
        if (i >= PICOMQTT_MAX_SUBSCRIPTIONS_PER_PACKET) {
            // Too many subscriptions requested in one packet.
            // Report subscription failure in SUBACK for the remaining ones.
            suback.write_u8(0x80);
        } else if (suback_codes[i >> 3] & (1 << (i & 7))) {
            // Subscription rejected due to topic filter too long.
            suback.write_u8(0x80);
        } else {
            // Subscription accepted with QoS 0.
            suback.write_u8(0x00);
        }
    }
    suback.send();
}

void Server::Client::on_unsubscribe(IncomingPacket & unsubscribe) {
    TRACE_FUNCTION;
    const uint16_t message_id = unsubscribe.read_u16();

    if ((unsubscribe.get_flags() != 0b0010) || !message_id) {
        on_protocol_violation();
        return;
    }

    while (unsubscribe.get_remaining_size()) {
        const size_t topic_size = unsubscribe.read_u16();
        if (topic_size > PICOMQTT_MAX_TOPIC_SIZE) {
            unsubscribe.ignore(topic_size);
        } else {
            char topic[topic_size + 1];
            if (!unsubscribe.read_string(topic, topic_size)) {
                // connection error
                return;
            }
            server.on_unsubscribe(client_id.c_str(), topic);
            this->unsubscribe(topic);
        }
    }

    auto unsuback = build_packet(Packet::UNSUBACK, 0, 2);
    unsuback.write_u16(message_id);
    unsuback.send();
}

Server::Client::SubscriptionId Server::Client::subscribe(
    const String & topic_filter) {
    TRACE_FUNCTION;
    if (!is_valid_topic_filter(topic_filter.c_str())) {
        return nullptr;
    }
    unsubscribe(topic_filter);
    Subscription * node = new Subscription(topic_filter.c_str());
    insert_subscription(node);
    return node;
}

void Server::Client::handle_packet(IncomingPacket & packet) {
    TRACE_FUNCTION;

    switch (packet.get_type()) {
        case Packet::PINGREQ:
            build_packet(Packet::PINGRESP).send();
            return;

        case Packet::SUBSCRIBE:
            on_subscribe(packet);
            return;

        case Packet::UNSUBSCRIBE:
            on_unsubscribe(packet);
            return;

        default:
            Connection::handle_packet(packet);
            return;
    }
}

void Server::Client::loop() {
    TRACE_FUNCTION;
    if (keep_alive_millis &&
        (get_millis_since_last_read() > keep_alive_millis)) {
        // ping timeout
        on_timeout();
        return;
    }

    Connection::loop();
}

Server::IncomingPublish::IncomingPublish(IncomingPacket & packet,
                                         Publish & publish)
    : IncomingPacket(std::move(packet)), publish(publish) {
    TRACE_FUNCTION;
}

Server::IncomingPublish::~IncomingPublish() {
    TRACE_FUNCTION;
    pos += publish.write_from_client(client, get_remaining_size());
}

int Server::IncomingPublish::read(uint8_t * buf, size_t size) {
    TRACE_FUNCTION;
    const int ret = IncomingPacket::read(buf, size);
    if (ret > 0) {
        publish.write(buf, ret);
    }
    return ret;
}

int Server::IncomingPublish::read() {
    TRACE_FUNCTION;
    const int ret = IncomingPacket::read();
    if (ret >= 0) {
        publish.write(ret);
    }
    return ret;
}

Server::Server(std::unique_ptr<ServerSocketInterface> server)
    : keep_alive_tolerance_millis(10 * 1000),
      socket_timeout_millis(5 * 1000),
      server(std::move(server)),
      clients(nullptr),
      print_mux(*this) {
    TRACE_FUNCTION;
}

Server::~Server() {
    Client * current = clients;
    while (current) {
        Client * next = current->next;
        delete current;
        current = next;
    }
}

void Server::begin() {
    TRACE_FUNCTION;
    server->begin();
}

void Server::loop() {
    TRACE_FUNCTION;

    ::Client * client_ptr = server->accept_client();
    if (client_ptr) {
        Client * client = new Client(*this, client_ptr);
        client->next = clients;
        clients = client;
        on_connected(client->get_client_id());
    }

    Client ** current = &clients;
    while (*current) {
        Client * client = *current;
        client->loop();

        if (!client->connected()) {
            on_disconnected(client->get_client_id());
            *current = client->next;
            delete client;
        } else {
            current = &client->next;
        }
    }
}

bool Server::set_subscribed(const char * topic) {
    TRACE_FUNCTION;
    bool any_subscribed = false;
    for (Client * client = clients; client; client = client->next) {
        client->subscribed = (client->get_subscription(topic) != nullptr);
        any_subscribed |= client->subscribed;
    }
    return any_subscribed;
}

Publisher::Publish Server::begin_publish(const char * topic,
                                         const size_t payload_size, uint8_t,
                                         bool, uint16_t) {
    TRACE_FUNCTION;
    set_subscribed(topic);
    return Publish(*this, print_mux, topic, payload_size);
}

void Server::on_message(const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION;
    fire_message_callbacks(topic, packet);
}

bool ServerLocalSubscribe::publish(const char * topic, const void * payload,
                                   const size_t payload_size, uint8_t qos,
                                   bool retain, uint16_t message_id) {
    TRACE_FUNCTION;
    const bool ret =
        Server::publish(topic, payload, payload_size, qos, retain, message_id);
    BufferClient buffer(payload);
    IncomingPacket packet(IncomingPacket::PUBLISH, 0, payload_size, buffer);
    fire_message_callbacks(topic, packet);
    return ret;
}

bool ServerLocalSubscribe::publish_P(const char * topic, PGM_P payload,
                                     const size_t payload_size, uint8_t qos,
                                     bool retain, uint16_t message_id) {
    TRACE_FUNCTION;
    const bool ret = Server::publish_P(topic, payload, payload_size, qos,
                                       retain, message_id);
    BufferClientP buffer((void *)payload);
    IncomingPacket packet(IncomingPacket::PUBLISH, 0, payload_size, buffer);
    fire_message_callbacks(topic, packet);
    return ret;
}

}  // namespace PicoMQTT
