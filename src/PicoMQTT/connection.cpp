#include "config.h"
#include "connection.h"
#include "debug.h"

namespace PicoMQTT {

Connection::Connection(unsigned long keep_alive_seconds, unsigned long socket_timeout_seconds) :
    client(socket_timeout_seconds),
    keep_alive_millis(keep_alive_seconds * 1000),
    last_read(millis()), last_write(millis()) {
    TRACE_FUNCTION
}

Connection::Connection(const ::WiFiClient & client, unsigned long keep_alive_seconds,
                       unsigned long socket_timeout_seconds) :
    client(client, socket_timeout_seconds),
    keep_alive_millis(keep_alive_seconds * 1000),
    last_read(millis()), last_write(millis()) {
    TRACE_FUNCTION
}

OutgoingPacket Connection::build_packet(Packet::Type type, uint8_t flags, size_t length) {
    TRACE_FUNCTION
    last_write = millis();
    auto ret = OutgoingPacket(client, type, flags, length);
    ret.write_header();
    return ret;
}

void Connection::on_timeout() {
    TRACE_FUNCTION
    client.abort();
    on_disconnect();
}

void Connection::on_protocol_violation() {
    TRACE_FUNCTION
    on_disconnect();
}

void Connection::on_disconnect() {
    TRACE_FUNCTION
    client.stop();
}

void Connection::disconnect() {
    TRACE_FUNCTION
    build_packet(Packet::DISCONNECT).send();
    client.stop();
}

bool Connection::connected() {
    TRACE_FUNCTION
    return client.connected();
}

void Connection::wait_for_reply(Packet::Type type, std::function<void(IncomingPacket & packet)> handler) {
    TRACE_FUNCTION

    const unsigned long start = millis();

    while (client.connected() && (millis() - start < client.socket_timeout_millis)) {

        IncomingPacket packet(client);
        if (!packet) {
            break;
        }

        last_read = millis();

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

void Connection::send_ack(Packet::Type ack_type, uint16_t msg_id) {
    TRACE_FUNCTION
    auto ack = build_packet(ack_type, 0, 2);
    ack.write_u16(msg_id);
    ack.send();
}

void Connection::handle_packet(IncomingPacket & packet) {
    TRACE_FUNCTION

    switch (packet.get_type()) {
        case Packet::PUBLISH: {
            const uint16_t topic_size = packet.read_u16();

            // const bool dup = (packet.get_flags() >> 3) & 0b1;
            const uint8_t qos = (packet.get_flags() >> 1) & 0b11;
            // const bool retain = packet.get_flags() & 0b1;

            uint16_t msg_id = 0;

            if (topic_size > PICOMQTT_MAX_TOPIC_SIZE) {
                packet.ignore(topic_size);
                on_topic_too_long(packet);
                if (qos) {
                    msg_id = packet.read_u16();
                }
            } else {
                char topic[topic_size + 1];
                if (!packet.read_string(topic, topic_size)) {
                    // connection error
                    return;
                }
                if (qos) {
                    msg_id = packet.read_u16();
                }
                on_message(topic, packet);
            }

            if (msg_id) {
                send_ack(qos == 1 ? Packet::PUBACK : Packet::PUBREC, msg_id);
            }

            break;
        };

        case Packet::PUBREC:
            send_ack(Packet::PUBREL, packet.read_u16());
            break;

        case Packet::PUBREL:
            send_ack(Packet::PUBCOMP, packet.read_u16());
            break;

        case Packet::PUBCOMP:
            // ignore
            break;

        case Packet::DISCONNECT:
            on_disconnect();
            break;

        default:
            on_protocol_violation();
            break;
    }
}

unsigned long Connection::get_millis_since_last_read() const {
    TRACE_FUNCTION
    return millis() - last_read;
}

unsigned long Connection::get_millis_since_last_write() const {
    TRACE_FUNCTION
    return millis() - last_write;
}

void Connection::loop() {
    TRACE_FUNCTION

    // only handle 10 packets max in one go to not starve other connections
    for (unsigned int i = 0; (i < 10) && client.available(); ++i) {
        IncomingPacket packet(client);
        if (!packet.is_valid()) {
            return;
        }
        last_read = millis();
        handle_packet(packet);
    }
}

}
