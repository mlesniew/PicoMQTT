#include "incoming_packet.h"
#include "debug.h"

namespace PicoMQTT {

IncomingPacket::IncomingPacket(Client & client)
    : Packet(read_header(client)), client(client) {
    TRACE_FUNCTION
}

IncomingPacket::IncomingPacket(IncomingPacket && other)
    : Packet(other), client(other.client) {
    TRACE_FUNCTION
    other.pos = size;
}

IncomingPacket::~IncomingPacket() {
    TRACE_FUNCTION
#ifdef PICOMQTT_DEBUG
    if (pos != size) {
        Serial.print(F("IncomingPacket read incorrect number of bytes: "));
        Serial.print(pos);
        Serial.print(F("/"));
        Serial.println(size);
    }
#endif
    // read and ignore remaining data
    while (get_remaining_size() && (read() >= 0));
}

// disabled functions
int IncomingPacket::connect(IPAddress ip, uint16_t port) {
    TRACE_FUNCTION;
    return 0;
}

int IncomingPacket::connect(const char * host, uint16_t port) {
    TRACE_FUNCTION;
    return 0;
}

size_t IncomingPacket::write(const uint8_t * buffer, size_t size) {
    TRACE_FUNCTION
    return 0;
}

size_t IncomingPacket::write(uint8_t value) {
    TRACE_FUNCTION
    return 0;
}

void IncomingPacket::flush() {
    TRACE_FUNCTION
}

void IncomingPacket::stop() {
    TRACE_FUNCTION
}


// extended functions
int IncomingPacket::available() {
    TRACE_FUNCTION;
    return get_remaining_size();
}

int IncomingPacket::peek() {
    TRACE_FUNCTION
    if (!get_remaining_size()) {
#if PICOMQTT_DEBUG
        Serial.println(F("Attempt to peek beyond end of IncomingPacket."));
#endif
        return -1;
    }
    return client.peek();
}

int IncomingPacket::read() {
    TRACE_FUNCTION
    if (!get_remaining_size()) {
#if PICOMQTT_DEBUG
        Serial.println(F("Attempt to read beyond end of IncomingPacket."));
#endif
        return -1;
    }
    const int ret = client.read();
    if (ret >= 0) {
        ++pos;
    }
    return ret;
}

int IncomingPacket::read(uint8_t * buf, size_t size) {
    TRACE_FUNCTION
    const size_t remaining = get_remaining_size();
    const size_t read_size = remaining < size ? remaining : size;
#if PICOMQTT_DEBUG
    if (size > remaining) {
        Serial.println(F("Attempt to read buf beyond end of IncomingPacket."));
    }
#endif
    const int ret = client.read(buf, read_size);
    if (ret > 0) {
        pos += ret;
    }
    return ret;
}

IncomingPacket::operator bool() {
    TRACE_FUNCTION
    return is_valid() && bool(client);
}

uint8_t IncomingPacket::connected() {
    TRACE_FUNCTION
    return is_valid() && client.connected();
}

// extra functions
uint8_t IncomingPacket::read_u8() {
    TRACE_FUNCTION;
    return get_remaining_size() ? read() : 0;
}

uint16_t IncomingPacket::read_u16() {
    TRACE_FUNCTION;
    return ((uint16_t) read_u8()) << 8 | ((uint16_t) read_u8());
}

bool IncomingPacket::read_string(char * buffer, size_t len) {
    if (read((uint8_t *) buffer, len) != (int) len) {
        return false;
    }
    buffer[len] = '\0';
    return true;
}

void IncomingPacket::ignore(size_t len) {
    while (len--) {
        read();
    }
}

Packet IncomingPacket::read_header(Client & client) {
    TRACE_FUNCTION
    const int head = client.read();
    if (head <= 0) {
        return Packet();
    }

    uint32_t size = 0;
    for (size_t length_size = 0; ; ++length_size) {
        if (length_size >= 5) {
            return Packet();
        }
        const int digit = client.read();
        if (digit < 0) {
            return Packet();
        }

        size |= (digit & 0x7f) << (7 * length_size);

        if (!(digit & 0x80)) {
            break;
        }
    }

    return Packet(head, size);
}

}
