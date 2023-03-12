#include "mqtt_connection.h"
#include "mqtt_debug.h"

namespace NanoMQTT {

Connection::Connection(
    ::Client & client, Buffer & buffer,
    unsigned long keep_alive_seconds, unsigned long socket_timeout_seconds) :
    client(client, socket_timeout_seconds),
    buffer(buffer),
    keep_alive_millis(keep_alive_seconds * 1000) {
    TRACE_FUNCTION
}

OutgoingPacket Connection::build_packet(Packet::Type type, uint8_t flags, size_t length) {
    TRACE_FUNCTION
    auto ret = OutgoingPacket(client, buffer, type, flags, length);
    ret.write_header();
    return ret;
}

void Connection::on_timeout() {
    TRACE_FUNCTION
    client.stop();
}

void Connection::on_protocol_violation() {
    TRACE_FUNCTION
    client.stop();
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

void Connection::on_message_too_big(IncomingPacket & packet) {
    TRACE_FUNCTION
}

void Connection::on_message(const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION
}

void Connection::handle_packet(IncomingPacket & packet) {
    TRACE_FUNCTION

    switch (packet.get_type()) {
        case Packet::PUBLISH: {
            buffer.reset();

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

        case Packet::DISCONNECT:
            on_disconnect();
            return;

        default:
            on_protocol_violation();
            return;
    }
}

void Connection::loop() {
    TRACE_FUNCTION

    // only handle 10 packets max in one go to not starve other connections
    for (unsigned int i = 0; (i < 10) && client.available(); ++i) {
        IncomingPacket packet(client);
        if (!packet.is_valid()) {
            return;
        }
        handle_packet(packet);
    }
}

}
