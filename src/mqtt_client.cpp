#include "incoming_packet.h"
#include "nanomqtt.h"
#include "mqtt_debug.h"

namespace NanoMQTT {

Client::Client(::Client & client, size_t buffer_size, uint16_t keep_alive_millis) :
    client(client, 15 * 1000),
    buffer(buffer_size),
    keep_alive_millis(keep_alive_millis) {
    TRACE_FUNCTION
}

OutgoingPacket Client::build_packet(Packet::Type type, uint8_t flags, size_t length) {
    TRACE_FUNCTION
    return OutgoingPacket(client, buffer, type, flags, length);
}

void Client::on_message_too_big(IncomingPacket & packet) {
    TRACE_FUNCTION
}

void Client::on_message(const char * topic, const char * payload, size_t payload_size) {
    TRACE_FUNCTION
}

void Client::on_message(const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION
    const auto payload = buffer.read(packet, packet.get_remaining_size());
    if (buffer.is_overflown()) {
        on_message_too_big(packet);
    } else {
        on_message(topic, payload.buffer, payload.size);
    }
}

void Client::on_timeout() {
    TRACE_FUNCTION
    client.stop();
}

void Client::on_protocol_violation() {
    TRACE_FUNCTION
    client.stop();
}

void Client::on_disconnect() {
    TRACE_FUNCTION
    client.stop();
}

bool Client::connect(
    const char * host,
    uint16_t port,
    const char * id,
    const char * user,
    const char * pass,
    const char * will_topic,
    const char * will_message,
    const size_t will_message_length,
    uint8_t will_qos,
    bool will_retain,
    const bool clean_session,
    ConnectReturnCode * connect_return_code) {
    TRACE_FUNCTION

    if (connect_return_code) {
        *connect_return_code = crc_undefined;
    }

    client.stop();

    if (!client.connect(host, port)) {
        return false;
    }

    messageIdGenerator.reset();

    const bool will = will_topic && will_message;

    const uint8_t connect_flags =
        (user ? 1 : 0) << 7
        | (user && pass ? 1 : 0) << 6
        | (will && will_retain ? 1 : 0) << 5
        | (will && will_qos ? 1 : 0) << 3
        | (will ? 1 : 0) << 2
        | (clean_session ? 1 : 0) << 1;

    const size_t client_id_length = strlen(id);
    const size_t will_topic_length = (will && will_topic) ? strlen(will_topic) : 0;
    const size_t user_length = user ? strlen(user) : 0;
    const size_t pass_length = pass ? strlen(pass) : 0;

    const size_t total_size = 6     // protocol name
                              + 1                         // protocol level
                              + 1                         // connect flags
                              + 2                         // keep-alive
                              + client_id_length + 2
                              + (will ? will_topic_length + 2 : 0)
                              + (will ? will_message_length + 2 : 0)
                              + (user ? user_length + 2 : 0)
                              + (user && pass ? pass_length + 2 : 0);

    auto packet = build_packet(Packet::CONNECT, 0, total_size);
    packet.write_string("MQTT", 4);
    packet.write_u8(4);
    packet.write_u8(connect_flags);
    packet.write_u16(keep_alive_millis / 1000);
    packet.write_string(id, client_id_length);

    if (will) {
        packet.write_string(will_topic, will_topic_length);
        packet.write_string(will_message, will_message_length);
    }

    if (user) {
        packet.write_string(user, user_length);
        if (pass)  {
            packet.write_string(pass, pass_length);
        }
    }

    if (!packet.send()) {
        return false;
    }

    wait_for_reply(Packet::CONNACK, [this, connect_return_code](IncomingPacket & packet) {
        TRACE_FUNCTION
        if (packet.size != 2) {
            on_protocol_violation();
            return;
        }

        /* const uint8_t connect_ack_flags = */ packet.read_u8();
        const uint8_t crc = packet.read_u8();

        if (connect_return_code) {
            *connect_return_code = (ConnectReturnCode) crc;
        }

        if (crc != 0) {
            // connection refused
            client.stop();
        }
    });

    return client.connected();
}

bool Client::send_simple_packet(Packet::Type type, uint8_t flags) {
    TRACE_FUNCTION
    return build_packet(type, flags, 0).send();
}

void Client::handle_packet(IncomingPacket & packet) {
    TRACE_FUNCTION

    buffer.reset();

    switch (packet.get_type()) {
        case Packet::CONNECT:
            // only allowed on servers
            on_protocol_violation();
            return;

        case Packet::CONNACK:
            // ok only after connect
            on_protocol_violation();
            return;

        case Packet::PUBLISH: {
            const char * topic = buffer.read_string(packet, packet.read_u16());

            // const bool dup = (packet.get_flags() >> 3) & 0b1;
            const uint8_t qos = (packet.get_flags() >> 1) & 0b11;
            // const bool retain = packet.get_flags() & 0b1;

            const bool ack_needed = (qos == 1);
            const uint16_t msg_id = ack_needed ? packet.read_u16() : 0;

            if (buffer.is_overflown()) {
                on_message_too_big(packet);
            } else {
                on_message(topic, packet);
            }

            if (ack_needed) {
                auto packet = build_packet(Packet::PUBACK, 0, 2);
                packet.write_u16(msg_id);
                packet.send();
            }
        };
        return;

        case Packet::PUBACK:
            // only allowed after publish
            on_protocol_violation();
            return;

        case Packet::PUBREC:
        case Packet::PUBREL:
        case Packet::PUBCOMP:
            // we should never see these messages since we only subscribe with QoS 0 and 1
            on_protocol_violation();
            return;

        case Packet::SUBSCRIBE:
        case Packet::UNSUBSCRIBE:
            on_protocol_violation();
            return;

        case Packet::SUBACK:
        case Packet::UNSUBACK:
            // only allowed after matching subscribe / unsubscribe
            on_protocol_violation();
            return;

        case Packet::PINGREQ:
            return;

        case Packet::PINGRESP:
            // only allowed after ping request
            on_protocol_violation();
            return;

        case Packet::DISCONNECT:
            on_disconnect();
            return;

        default:
            on_protocol_violation();
            return;
    }
}

void Client::wait_for_reply(Packet::Type type, std::function<void(IncomingPacket & packet)> handler) {
    TRACE_FUNCTION

    const unsigned long start = millis();

    while (client.connected() && (millis() - start < client.read_timeout_millis)) {

        IncomingPacket packet(client);
        if (!packet) {
            break;
        }

        if (packet.get_type() == type) {
            handler(packet);
            return;
        }

        handle_packet(packet);

    }

    if (client.connected()) {
        on_timeout();
    }
}

void Client::loop() {
    TRACE_FUNCTION

    if (!client.connected()) {
        return;
    }

    if (client.get_millis_since_last_write() >= keep_alive_millis) {
        // ping time!
        send_simple_packet(Packet::PINGREQ);
        wait_for_reply(Packet::PINGRESP, [](IncomingPacket &) {});
    }

    while (client.available()) {
        IncomingPacket packet(client);
        if (!packet) {
            return;
        }

        handle_packet(packet);
    }
}

bool Client::confirm_publish(const Client::Publish & publish) {
    TRACE_FUNCTION
    if (publish.qos == 0) {
        return true;
    }

    bool confirmed = false;
    wait_for_reply(Packet::PUBACK, [&publish, &confirmed](IncomingPacket & puback) {
        confirmed |= (puback.read_u16() == publish.message_id);
    });

    return confirmed;
}

Client::Publish::Publish(Client & mqtt,
                         const char * topic, size_t topic_size, size_t payload_size,
                         uint8_t qos, bool retain, bool dup)
    :
    OutgoingPacket(
        mqtt.client,
        mqtt.buffer,
        Packet::PUBLISH,
        (dup ? 0b1000 : 0) | ((qos & 0b11) << 1) | (retain ? 1 : 0),
        2 + topic_size + (qos ? 2 : 0) + payload_size),
    qos(qos),
    message_id(qos ? mqtt.messageIdGenerator.generate() : 0),
    mqtt(mqtt) {
    TRACE_FUNCTION

    write_string(topic, topic_size);
    if (qos) {
        write_u16(message_id);
    }
}

bool Client::Publish::send() {
    TRACE_FUNCTION
    return OutgoingPacket::send() && mqtt.confirm_publish(*this);
}

Client::Publish Client::publish(const char * topic, const size_t payload_size,
                                uint8_t qos, bool retain, bool dup) {
    TRACE_FUNCTION
    return Publish(
               *this,
               topic, strlen(topic), payload_size,
               (qos >= 1) ? 1 : 0, retain, dup);
}

bool Client::publish(const char * topic, const void * payload, const size_t payload_size,
                     uint8_t qos, bool retain, bool dup) {
    TRACE_FUNCTION
    auto packet = publish(topic, payload_size, qos, retain, dup);
    packet.write(payload, payload_size);
    return packet.send();
}

bool Client::publish(const char * topic, const char * payload,
                     uint8_t qos, bool retain, bool dup) {
    TRACE_FUNCTION
    return publish(topic, payload, strlen(payload), qos, retain, dup);
}

bool Client::publish(const String & topic, const String & payload,
                     uint8_t qos, bool retain, bool dup) {
    TRACE_FUNCTION
    return publish(topic.c_str(), payload.c_str(), qos, retain, dup);
}

bool Client::publish_P(const char * topic, const void * payload, const size_t payload_size,
                       uint8_t qos, bool retain, bool dup) {
    TRACE_FUNCTION;
    auto packet = publish(topic, payload_size, qos, retain, dup);
    packet.write_P(payload, payload_size);
    return packet.send();
}

bool Client::publish_P(const char * topic, const char * payload,
                       uint8_t qos, bool retain, bool dup) {
    TRACE_FUNCTION
    return publish_P(topic, payload, strlen_P(payload), qos, retain, dup);
}

bool Client::subscribe(const String & topic, uint8_t qos, uint8_t * qos_granted) {
    TRACE_FUNCTION
    if (qos > 1) {
        return false;
    }

    const size_t topic_size = topic.length();
    const uint16_t message_id = messageIdGenerator.generate();

    auto packet = build_packet(Packet::SUBSCRIBE, 0b0010, 2 + 2 + topic_size + 1);
    packet.write_u16(message_id);
    packet.write_string(topic.c_str(), topic_size);
    packet.write_u8(qos);
    packet.send();

    uint8_t code = 0x80;

    wait_for_reply(Packet::SUBACK, [this, message_id, &code](IncomingPacket & packet) {
        if (packet.read_u16() != message_id) {
            on_protocol_violation();
        } else {
            code = packet.read_u8();
        }
    });

    if (code == 0x80) {
        return false;
    }

    if (qos_granted) {
        *qos_granted = code;
    }

    return client.connected();
}

bool Client::unsubscribe(const String & topic) {
    TRACE_FUNCTION

    const size_t topic_size = topic.length();
    const uint16_t message_id = messageIdGenerator.generate();

    auto packet = build_packet(Packet::UNSUBSCRIBE, 0b0010, 2 + 2 + topic_size);
    packet.write_u16(message_id);
    packet.write_string(topic.c_str(), topic_size);
    packet.send();

    wait_for_reply(Packet::UNSUBACK, [this, message_id](IncomingPacket & packet) {
        if (packet.read_u16() != message_id) {
            on_protocol_violation();
        }
    });

    return client.connected();
}

void Client::disconnect() {
    TRACE_FUNCTION
    send_simple_packet(Packet::DISCONNECT);
    client.stop();
}

bool Client::connected() {
    TRACE_FUNCTION
    return client.connected();
}

}
