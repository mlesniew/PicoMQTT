#include <Client.h>
#include <Print.h>

#include "outgoing_packet.h"
#include "mqtt_debug.h"

namespace NanoMQTT {

OutgoingPacket::OutgoingPacket(Print & print, Buffer & buffer, Packet::Type type, uint8_t flags, size_t payload_size)
    : Packet(type, flags, payload_size), print(print), buffer(buffer), state(State::ok) {
    TRACE_FUNCTION
    buffer.reset();
}

OutgoingPacket::OutgoingPacket(OutgoingPacket && other)
    : OutgoingPacket(other) {
    TRACE_FUNCTION
    other.state = State::dead;
}

OutgoingPacket::~OutgoingPacket() {
    TRACE_FUNCTION
#ifdef MQTT_DEBUG
    switch (state) {
        case State::ok:
            Serial.println(F("Unsent OutgoingPacket"));
            break;
        case State::sent:
            if (pos != size) {
                Serial.print(F("OutgoingPacket sent incorrect number of bytes: "));
                Serial.print(pos);
                Serial.print(F("/"));
                Serial.println(size);
            }
            break;
        default:
            break;
    }
#endif
}

size_t OutgoingPacket::write_from_client(::Client & client, size_t length) {
    TRACE_FUNCTION

    size_t written = 0;

    while (written < length) {
        const auto alloc = buffer.allocate(length - written);

        if (!client.read((uint8_t *) alloc.buffer, alloc.size)) {
            break;
        }

        pos += alloc.size;
        written += alloc.size;

        if (!buffer.get_remaining_size()) {
            flush();
        }
    }

    return written;
}

size_t OutgoingPacket::write_zero(size_t length) {
    TRACE_FUNCTION
    for (size_t written = 0; written < length; ++written) {
        write_u8(0);
    }
    return length;
}

size_t OutgoingPacket::write(const void * data, size_t length, void * (*memcpy_fn)(void *, const void *, size_t n)) {
    TRACE_FUNCTION

    const char * src = (const char *) data;
    while ((state == State::ok) && length) {

        const auto alloc = buffer.allocate(length);

        memcpy_fn(alloc.buffer, src, alloc.size);
        src += alloc.size;
        length -= alloc.size;
        pos += alloc.size;

        if (!buffer.get_remaining_size()) {
            flush();
        }
    }
    return src - (const char *) data;
}

size_t OutgoingPacket::write(const void * data, size_t length) {
    TRACE_FUNCTION
    return write(data, length, memcpy);
}

size_t OutgoingPacket::write_P(const void * data, size_t length) {
    TRACE_FUNCTION
    return write(data, length, memcpy_P);
}

size_t OutgoingPacket::write_u8(uint8_t c) {
    TRACE_FUNCTION
    return write(&c, 1);
}

size_t OutgoingPacket::write_u16(uint16_t value) {
    TRACE_FUNCTION
    return write_u8(value >> 8) + write_u8(value & 0xff);
}

size_t OutgoingPacket::write_string(const char * string, uint16_t size) {
    TRACE_FUNCTION
    return write_u16(size) + write(string, size);
}

size_t OutgoingPacket::write_packet_length(size_t length) {
    TRACE_FUNCTION
    size_t ret = 0;
    do {
        const uint8_t digit = length & 127; // digit := length % 128
        length >>= 7;                       // length := length / 128
        ret += write_u8(digit | (length ? 0x80 : 0));
    } while (length);
    return ret;
}

size_t OutgoingPacket::write_header() {
    TRACE_FUNCTION
    const size_t ret = write_u8(head) + write_packet_length(size);
    // we've just written the header, payload starts now
    pos = 0;
    return ret;
}

void OutgoingPacket::flush() {
    TRACE_FUNCTION
    const auto alloc = buffer.get_content();
    size_t written = 0;
    while ((state == State::ok) && (written < alloc.size)) {
        const size_t w = print.write((const uint8_t *)(alloc.buffer + written), alloc.size - written);
        if (!w) {
            state = State::error;
            break;
        }
        written += w;
    }
    buffer.reset();
}

bool OutgoingPacket::send() {
    TRACE_FUNCTION
    flush();
    switch (state) {
        case State::ok:
            print.flush();
            state = State::sent;
        case State::sent:
            return true;
        default:
            return false;
    }
}

}
