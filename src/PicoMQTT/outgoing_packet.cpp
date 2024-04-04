#include <Client.h>
#include <Print.h>

#include "debug.h"
#include "outgoing_packet.h"

namespace PicoMQTT {

OutgoingPacket::OutgoingPacket(Print & print, Packet::Type type, uint8_t flags, size_t payload_size)
    : Packet(type, flags, payload_size), print(print),
#ifndef PICOMQTT_UNBUFFERED
      buffer_position(0),
#endif
      state(State::ok)  {
    TRACE_FUNCTION
}

OutgoingPacket::OutgoingPacket(OutgoingPacket && other)
    : OutgoingPacket(other) {
    TRACE_FUNCTION
    other.state = State::dead;
}

OutgoingPacket::~OutgoingPacket() {
    TRACE_FUNCTION
#ifdef PICOMQTT_DEBUG
#ifndef PICOMQTT_UNBUFFERED
    if (buffer_position) {
        Serial.printf("OutgoingPacket has unsent data in the buffer (pos=%u)\n", buffer_position);
    }
#endif
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
#ifndef PICOMQTT_UNBUFFERED
    while (written < length) {
        const size_t remaining = length - written;
        const size_t remaining_buffer_space = PICOMQTT_OUTGOING_BUFFER_SIZE - buffer_position;
        const size_t chunk_size = remaining < remaining_buffer_space ? remaining : remaining_buffer_space;

        const int read_size = client.read(buffer + buffer_position, chunk_size);
        if (read_size <= 0) {
            break;
        }

        buffer_position += (size_t) read_size;
        written += (size_t) read_size;

        if (buffer_position >= PICOMQTT_OUTGOING_BUFFER_SIZE) {
            flush();
        }
    }
#else
    uint8_t buffer[128] __attribute__((aligned(4)));
    while (written < length) {
        const size_t remain = length - written;
        const size_t chunk_size = sizeof(buffer) < remain ? sizeof(buffer) : remain;
        const int read_size = client.read(buffer, chunk_size);
        if (read_size <= 0) {
            break;
        }
        const size_t write_size = print.write(buffer, read_size);
        written += write_size;
        if (!write_size) {
            break;
        }
    }
#endif
    pos += written;
    return written;
}

size_t OutgoingPacket::write_zero(size_t length) {
    TRACE_FUNCTION
    for (size_t written = 0; written < length; ++written) {
        write_u8('0');
    }
    return length;
}

#ifndef PICOMQTT_UNBUFFERED
size_t OutgoingPacket::write(const void * data, size_t remaining, void * (*memcpy_fn)(void *, const void *, size_t n)) {
    TRACE_FUNCTION

    const char * src = (const char *) data;

    while (remaining) {
        const size_t remaining_buffer_space = PICOMQTT_OUTGOING_BUFFER_SIZE - buffer_position;
        const size_t chunk_size = remaining < remaining_buffer_space ? remaining : remaining_buffer_space;

        memcpy_fn(buffer + buffer_position, src, chunk_size);

        buffer_position += chunk_size;
        src += chunk_size;
        remaining -= chunk_size;

        if (buffer_position >= PICOMQTT_OUTGOING_BUFFER_SIZE) {
            flush();
        }
    }

    const size_t written = src - (const char *) data;
    pos += written;
    return written;
}
#endif

size_t OutgoingPacket::write(const uint8_t * data, size_t length) {
    TRACE_FUNCTION
#ifndef PICOMQTT_UNBUFFERED
    return write(data, length, memcpy);
#else
    const size_t written = print.write(data, length);
    pos += written;
    return written;
#endif
}

size_t OutgoingPacket::write_P(PGM_P data, size_t length) {
    TRACE_FUNCTION
#ifndef PICOMQTT_UNBUFFERED
    return write(data, length, memcpy_P);
#else
    // here we will need a buffer
    uint8_t buffer[128] __attribute__((aligned(4)));
    size_t written = 0;
    while (written < length) {
        const size_t remain = length - written;
        const size_t chunk_size = sizeof(buffer) < remain ? sizeof(buffer) : remain;
        memcpy_P(buffer, data, chunk_size);
        const size_t write_size = print.write(buffer, chunk_size);
        written += write_size;
        data += write_size;
        if (!write_size) {
            break;
        }
    }
    pos += written;
    return written;
#endif
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
    return write_u16(size) + write((const uint8_t *) string, size);
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
#ifndef PICOMQTT_UNBUFFERED
    print.write(buffer, buffer_position);
    buffer_position = 0;
#endif
}

bool OutgoingPacket::send() {
    TRACE_FUNCTION
    const size_t remaining_size = get_remaining_size();
    if (remaining_size) {
#ifdef PICOMQTT_DEBUG
        Serial.printf("OutgoingPacket sent called on incomplete payload (%u / %u), filling with zeros.\n", pos, size);
#endif
        write_zero(remaining_size);
    }
    flush();
    switch (state) {
        case State::ok:
            // print.flush();
            state = State::sent;
            __attribute__((fallthrough));
        case State::sent:
            return true;
        default:
            return false;
    }
}

}
