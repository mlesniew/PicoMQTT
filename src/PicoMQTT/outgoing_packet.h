#pragma once

// #define MQTT_OUTGOING_PACKET_DEBUG

#include <Arduino.h>

#include "config.h"
#include "packet.h"

class Print;
class Client;

#if PICOMQTT_OUTGOING_BUFFER_SIZE == 0
#define PICOMQTT_UNBUFFERED
#endif

namespace PicoMQTT {

class OutgoingPacket: public Packet, public Print {
    public:
        OutgoingPacket(Print & print, Type type, uint8_t flags, size_t payload_size);
        virtual ~OutgoingPacket();

        const OutgoingPacket & operator=(const OutgoingPacket &) = delete;
        OutgoingPacket(OutgoingPacket && other);

        virtual size_t write(const uint8_t * data, size_t length) override;
        virtual size_t write(uint8_t value) override final { return write(&value, 1); }

        size_t write_P(PGM_P data, size_t length);
        size_t write_u8(uint8_t value);
        size_t write_u16(uint16_t value);
        size_t write_string(const char * string, uint16_t size);
        size_t write_header();

        size_t write_from_client(::Client & c, size_t length);
        size_t write_zero(size_t count);

        virtual void flush() override;
        virtual bool send();

    protected:
        OutgoingPacket(const OutgoingPacket &) = default;

        size_t write(const void * data, size_t length, void * (*memcpy_fn)(void *, const void *, size_t n));
        size_t write_packet_length(size_t length);

        Print & print;

#ifndef PICOMQTT_UNBUFFERED
        uint8_t buffer[PICOMQTT_OUTGOING_BUFFER_SIZE] __attribute__((aligned(4)));

        size_t buffer_position;
#endif

        enum class State {
            ok,
            sent,
            error,
            dead,
        } state;
};

}
