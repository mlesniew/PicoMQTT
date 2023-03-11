#pragma once

#include <Client.h>

namespace NanoMQTT {

class ClientWrapper: public ::Client {
    public:
        ClientWrapper(::Client & client, unsigned long read_timeout_millis);

        virtual int available() override;
        virtual int connect(IPAddress ip, uint16_t port) override;
        virtual int connect(const char * host, uint16_t port) override;
        virtual int peek() override;
        virtual int read() override;
        virtual int read(uint8_t * buf, size_t size) override;
        virtual operator bool() override;
        virtual size_t write(const uint8_t * buffer, size_t size) override;
        virtual size_t write(uint8_t value) override final;
        virtual uint8_t connected() override;
        virtual void flush() override;
        virtual void stop() override;

        unsigned long get_millis_since_last_write() const;
        const unsigned long read_timeout_millis;

    protected:
        int available_wait(unsigned long timeout);

        ::Client & client;
        unsigned long last_write;
};

}
