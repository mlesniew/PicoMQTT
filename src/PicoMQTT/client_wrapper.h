#pragma once

#include <WiFiClient.h>

#include "config.h"

namespace PicoMQTT {

class ClientWrapper: public ::Client {
    public:
        ClientWrapper(::Client & client, unsigned long socket_timeout_millis);
        ClientWrapper(const ClientWrapper &) = default;

        virtual int peek() override;
        virtual int read() override;
        virtual int read(uint8_t * buf, size_t size) override;
        virtual size_t write(const uint8_t * buffer, size_t size) override;
        virtual size_t write(uint8_t value) override final;

        // all of the below call the corresponding method on this->client
        virtual int connect(IPAddress ip, uint16_t port) override;
        virtual int connect(const char * host, uint16_t port) override;
#ifdef PICOMQTT_EXTRA_CONNECT_METHODS
        virtual int connect(IPAddress ip, uint16_t port, int32_t timeout) override;
        virtual int connect(const char * host, uint16_t port, int32_t timeout) override;
#endif
        virtual int available() override;
        virtual void flush() override;
        virtual void stop() override;
        virtual uint8_t connected() override;
        virtual operator bool() override;

        const unsigned long socket_timeout_millis;

        void abort() {
            // TODO: Use client.abort() if client is a WiFiClient on ESP8266?
            stop();
        }

    protected:
        ::Client & client;

        int available_wait(unsigned long timeout);
};

}
