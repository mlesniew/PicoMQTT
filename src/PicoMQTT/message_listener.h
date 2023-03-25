#pragma once

namespace PicoMQTT {

class IncomingPacket;

class MessageListener {
    public:
        virtual void on_message_too_big(const char * topic, IncomingPacket & packet) {}
        virtual void on_message(const char * topic, const char * payload, size_t payload_size) {}
        virtual void on_message(const char * topic, IncomingPacket & packet);
};

}
