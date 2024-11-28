#pragma once

#include <Arduino.h>
#include <Client.h>

#include "config.h"
#include "packet.h"

namespace PicoMQTT {

class IncomingPacket: public Packet, public Client {
    public:
        IncomingPacket(Client & client);
        IncomingPacket(const Type type, const uint8_t flags, const size_t size, Client & client);
        IncomingPacket(IncomingPacket &&);

        IncomingPacket(const IncomingPacket &) = delete;
        const IncomingPacket & operator=(const IncomingPacket &) = delete;

        ~IncomingPacket();

        virtual int available() override;
        virtual int connect(IPAddress ip, uint16_t port) override;
        virtual int connect(const char * host, uint16_t port) override;
#ifdef PICOMQTT_EXTRA_CONNECT_METHODS
        virtual int connect(IPAddress ip, uint16_t port, int32_t timeout) override;
        virtual int connect(const char * host, uint16_t port, int32_t timeout) override;
#endif
        virtual int peek() override;
        virtual int read() override;
        virtual int read(uint8_t * buf, size_t size) override;
        // This operator is not marked explicit in the Client base class.  Still, we're marking it explicit here
        // to block implicit conversions to integer types.
        virtual explicit operator bool() override;
        virtual size_t write(const uint8_t * buffer, size_t size) override;
        virtual size_t write(uint8_t value) override final;
        virtual uint8_t connected() override;
        virtual void flush() override;
        virtual void stop() override;

        uint8_t read_u8();
        uint16_t read_u16();
        bool read_string(char * buffer, size_t len);
        void ignore(size_t len);

    protected:
        static Packet read_header(Client & client);

        Client & client;
};

}
