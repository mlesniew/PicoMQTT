#include "mqtt_server.h"
#include "mqtt_debug.h"

namespace NanoMQTT {

Server::Client::Client(const Server::Client & other)
    : Connection(wifi_client, other.buffer, 0),
      Subscriber(other),
      server(other.server), wifi_client(other.wifi_client),
      client_id(other.client_id) {
    TRACE_FUNCTION
}

Server::Client::Client(Server & server, const WiFiClient & p_client)
    : Connection(wifi_client, server.buffer, 0, server.socket_timeout_seconds),
      server(server), wifi_client(p_client),
      client_id("<unknown>") {
    TRACE_FUNCTION
    wait_for_reply(Packet::CONNECT, [this](IncomingPacket & packet) {
        TRACE_FUNCTION

        buffer.reset();

        if (strcmp(buffer.read_string(packet, packet.read_u16()), "MQTT") != 0) {
            on_protocol_violation();
            return;
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

        const uint16_t keep_alive_seconds = packet.read_u16();
        keep_alive_millis = keep_alive_seconds ? (keep_alive_seconds + this->server.keep_alive_tolerance_seconds) * 1000 : 0;
        client_id = buffer.read_string(packet, packet.read_u16());

        if (client_id.isEmpty()) {
            client_id = String((unsigned int)(this), HEX);
        }

        if (has_will) {
            /* const char * will_topic = */ buffer.read_string(packet, packet.read_u16());
            /* const char * will_payload = */ buffer.read_string(packet, packet.read_u16());
        }

        const char * user = has_user ? buffer.read_string(packet, packet.read_u16()) : nullptr;
        const char * pass = has_pass ? buffer.read_string(packet, packet.read_u16()) : nullptr;

        const uint8_t connect_return_code = this->server.auth(client_id.c_str(), user, pass);

        auto reply = build_packet(Packet::CONNACK, 0, 2);
        reply.write_u8(0);  /* session present always set to zero */
        reply.write_u8(connect_return_code);
        reply.send();
    });
}

void Server::Client::on_message(const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION
    const size_t payload_size = packet.get_remaining_size();
    // Topic is allocated in the same buffer as the one we're going to use for the outgoing packet, let's allocate
    // a temporary variable to hold a copy of the topic.
    const String topic_string = topic;

    auto publish = server.publish(topic_string.c_str(), payload_size);

    const size_t written = publish.write_from_client(packet, payload_size);

    // in case client reads failed, fill the packet with zeros to not cause a protocol violation
    publish.write_zero(payload_size - written);

    publish.send();
}

void Server::Client::on_subscribe(IncomingPacket & subscribe) {
    TRACE_FUNCTION
    const uint16_t message_id = subscribe.read_u16();

    if ((subscribe.get_flags() != 0b0010) || !message_id) {
        on_protocol_violation();
        return;
    }

    std::list<uint8_t> suback_codes;

    while (subscribe.get_remaining_size()) {
        buffer.reset();
        const char * topic = buffer.read_string(subscribe, subscribe.read_u16());
        uint8_t qos = subscribe.read_u8();
        if (qos > 2) {
            on_protocol_violation();
            return;
        }

        uint8_t qos_granted = 0x80;

        // TODO: check if topic is valid
        if (!buffer.is_overflown()) {
            Subscriber::subscribe(topic);
            server.on_subscribe(client_id.c_str(), topic);
            qos_granted = 0;
        }

        suback_codes.push_back(qos_granted);
    }

    auto suback = build_packet(Packet::SUBACK, 0, 2 + suback_codes.size());
    suback.write_u16(message_id);
    for (uint8_t code : suback_codes) {
        suback.write_u8(code);
    }
    suback.send();
}

void Server::Client::on_unsubscribe(IncomingPacket & unsubscribe) {
    TRACE_FUNCTION
    const uint16_t message_id = unsubscribe.read_u16();

    if ((unsubscribe.get_flags() != 0b0010) || !message_id) {
        on_protocol_violation();
        return;
    }

    while (unsubscribe.get_remaining_size()) {
        buffer.reset();
        const char * topic = buffer.read_string(unsubscribe, unsubscribe.read_u16());
        if (!buffer.is_overflown()) {
            server.on_unsubscribe(client_id.c_str(), topic);
            Subscriber::unsubscribe(topic);
        }
    }

    auto unsuback = build_packet(Packet::UNSUBACK, 0, 2);
    unsuback.write_u16(message_id);
    unsuback.send();
}

void Server::Client::handle_packet(IncomingPacket & packet) {
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

void Server::Client::loop() {
    TRACE_FUNCTION
    if (keep_alive_millis && (client.get_millis_since_last_read() > keep_alive_millis)) {
        // ping timeout
        on_timeout();
        return;
    }

    Connection::loop();
}


Server::Server(uint16_t port, size_t buffer_size, unsigned long keep_alive_tolerance_seconds,
               unsigned long socket_timeout_seconds)
    : server(port), buffer(buffer_size), keep_alive_tolerance_seconds(keep_alive_tolerance_seconds),
      socket_timeout_seconds(socket_timeout_seconds) {
    TRACE_FUNCTION
}

void Server::begin() {
    TRACE_FUNCTION
    server.begin();
}

void Server::stop() {
    TRACE_FUNCTION
    server.stop();
    clients.clear();
}

void Server::loop() {
    TRACE_FUNCTION

    while (server.hasClient()) {
        auto client = Client(*this, server.accept());
        clients.push_back(client);
        on_connect(client.get_client_id());
    }

    for (auto it = clients.begin(); it != clients.end();) {
        it->loop();

        if (!it->connected()) {
            on_disconnect(it->get_client_id());
            clients.erase(it++);
        } else {
            ++it;
        }
    }
}

PrintMux Server::get_subscribed(const char * topic) {
    TRACE_FUNCTION
    PrintMux ret;
    for (auto & client : clients) {
        if (client.get_subscription(topic)) {
            ret.add(client.get_print());
        }
    }
    return ret;
}

Publisher::Publish Server::publish(const char * topic, const size_t payload_size,
                                   uint8_t, bool, uint16_t) {
    TRACE_FUNCTION
    return Publish(*this, get_subscribed(topic), buffer, topic, payload_size);
}

}
