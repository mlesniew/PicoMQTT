#include "config.h"
#include "debug.h"
#include "server.h"

namespace PicoMQTT {

BasicServer::Client::Client(const BasicServer::Client & other)
    : Connection(other.client, other.keep_alive_millis / 1000, other.client.socket_timeout_millis / 1000),
      server(other.server),
      client_id(other.client_id) {
    TRACE_FUNCTION
}

BasicServer::Client::Client(BasicServer & server, const WiFiClient & client)
    : Connection(client, 0, server.socket_timeout_seconds), server(server), client_id("<unknown>") {
    TRACE_FUNCTION
    wait_for_reply(Packet::CONNECT, [this](IncomingPacket & packet) {
        TRACE_FUNCTION

        auto connack = [this](ConnectReturnCode crc) {
            TRACE_FUNCTION
            auto connack = build_packet(Packet::CONNACK, 0, 2);
            connack.write_u8(0);  /* session present always set to zero */
            connack.write_u8(crc);
            connack.send();
            if (crc != CRC_ACCEPTED) {
                this->client.stop();
            }
        };

        {
            // MQTT protocol identifier
            char buf[4];

            if (packet.read_u16() != 4) {
                on_protocol_violation();
                return;
            }

            packet.read((uint8_t *) buf, 4);

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

        if ((has_pass && !has_user)
                || (will_qos > 2)
                || (!has_will && ((will_qos > 0) || will_retain))) {
            on_protocol_violation();
            return;
        }

        const unsigned long keep_alive_seconds = packet.read_u16();
        keep_alive_millis = keep_alive_seconds ? (keep_alive_seconds + this->server.keep_alive_tolerance_seconds) * 1000 : 0;

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
            packet.ignore(packet.read_u16()); // will topic
            packet.ignore(packet.read_u16()); // will payload
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

        const auto connect_return_code = this->server.auth(
                                             client_id.c_str(),
                                             has_user ? user : nullptr, has_pass ? pass : nullptr);

        connack(connect_return_code);
    });
}

void BasicServer::Client::on_message(const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION

    const size_t payload_size = packet.get_remaining_size();
    auto publish = server.begin_publish(topic, payload_size);

    // Always notify the server about the message
    {
        IncomingPublish incoming_publish(packet, publish);
        server.on_message(topic, incoming_publish);
    }

    publish.send();
}

void BasicServer::Client::on_subscribe(IncomingPacket & subscribe) {
    TRACE_FUNCTION
    const uint16_t message_id = subscribe.read_u16();

    if ((subscribe.get_flags() != 0b0010) || !message_id) {
        on_protocol_violation();
        return;
    }

    std::list<uint8_t> suback_codes;

    while (subscribe.get_remaining_size()) {
        const size_t topic_size = subscribe.read_u16();
        if (topic_size > PICOMQTT_MAX_TOPIC_SIZE) {
            subscribe.ignore(topic_size);
            subscribe.read_u8();
            suback_codes.push_back(0x80);
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
            this->subscribe(topic);
            server.on_subscribe(client_id.c_str(), topic);
            suback_codes.push_back(0);
        }
    }

    auto suback = build_packet(Packet::SUBACK, 0, 2 + suback_codes.size());
    suback.write_u16(message_id);
    for (uint8_t code : suback_codes) {
        suback.write_u8(code);
    }
    suback.send();
}

void BasicServer::Client::on_unsubscribe(IncomingPacket & unsubscribe) {
    TRACE_FUNCTION
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

const char * BasicServer::Client::get_subscription_pattern(BasicServer::Client::SubscriptionId id) const {
    for (const auto & pattern : subscriptions)
        if (pattern.id == id) {
            return pattern.c_str();
        }
    return nullptr;
}

Server::SubscriptionId BasicServer::Client::get_subscription(const char * topic) const {
    TRACE_FUNCTION
    for (const auto & pattern : subscriptions)
        if (topic_matches(pattern.c_str(), topic)) {
            return pattern.id;
        }
    return 0;
}

BasicServer::Client::SubscriptionId BasicServer::Client::subscribe(const String & topic_filter) {
    TRACE_FUNCTION
    const Subscription subscription(topic_filter.c_str());
    subscriptions.insert(subscription);
    return subscription.id;
}

void BasicServer::Client::unsubscribe(const String & topic_filter) {
    TRACE_FUNCTION
    subscriptions.erase(topic_filter.c_str());
}

void BasicServer::Client::handle_packet(IncomingPacket & packet) {
    TRACE_FUNCTION

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

void BasicServer::Client::loop() {
    TRACE_FUNCTION
    if (keep_alive_millis && (get_millis_since_last_read() > keep_alive_millis)) {
        // ping timeout
        on_timeout();
        return;
    }

    Connection::loop();
}

BasicServer::IncomingPublish::IncomingPublish(IncomingPacket & packet, Publish & publish)
    : IncomingPacket(std::move(packet)), publish(publish) {
    TRACE_FUNCTION
}

BasicServer::IncomingPublish::~IncomingPublish() {
    TRACE_FUNCTION
    pos += publish.write_from_client(client, get_remaining_size());
}

int BasicServer::IncomingPublish::read(uint8_t * buf, size_t size) {
    TRACE_FUNCTION
    const int ret = IncomingPacket::read(buf, size);
    if (ret > 0) {
        publish.write(buf, ret);
    }
    return ret;
}

int BasicServer::IncomingPublish::read() {
    TRACE_FUNCTION
    const int ret = IncomingPacket::read();
    if (ret >= 0) {
        publish.write(ret);
    }
    return ret;
}

BasicServer::BasicServer(uint16_t port, unsigned long keep_alive_tolerance_seconds,
                         unsigned long socket_timeout_seconds)
    : server(port), keep_alive_tolerance_seconds(keep_alive_tolerance_seconds),
      socket_timeout_seconds(socket_timeout_seconds) {
    TRACE_FUNCTION
}

void BasicServer::begin() {
    TRACE_FUNCTION
    server.begin();
}

void BasicServer::stop() {
    TRACE_FUNCTION
    server.stop();
    clients.clear();
}

void BasicServer::loop() {
    TRACE_FUNCTION

    while (server.hasClient()) {
        auto client = Client(*this, server.accept());
        clients.push_back(client);
        on_connected(client.get_client_id());
    }

    for (auto it = clients.begin(); it != clients.end();) {
        it->loop();

        if (!it->connected()) {
            on_disconnected(it->get_client_id());
            clients.erase(it++);
        } else {
            ++it;
        }
    }
}

PrintMux BasicServer::get_subscribed(const char * topic) {
    TRACE_FUNCTION
    PrintMux ret;
    for (auto & client : clients) {
        if (client.get_subscription(topic)) {
            ret.add(client.get_print());
        }
    }
    return ret;
}

Publisher::Publish BasicServer::begin_publish(const char * topic, const size_t payload_size,
        uint8_t, bool, uint16_t) {
    TRACE_FUNCTION
    return Publish(*this, get_subscribed(topic), topic, payload_size);
}

void BasicServer::on_message(const char * topic, IncomingPacket & packet) {
}

void Server::on_message(const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION
    fire_message_callbacks(topic, packet);
}

}
