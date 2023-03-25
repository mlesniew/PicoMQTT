#include "config.h"
#include "debug.h"
#include "incoming_packet.h"
#include "message_listener.h"

namespace PicoMQTT {

void MessageListener::on_message(const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION

    const size_t payload_size = packet.get_remaining_size();
    if (payload_size > PICOMQTT_MAX_MESSAGE_SIZE) {
        on_message_too_big(topic, packet);
        return;
    }

    char payload[payload_size];

    if (packet.read((uint8_t *) payload, payload_size) != (int) payload_size) {
        // connection loss or timeout; callbacks already fired
        return;
    }

    on_message(topic, payload, payload_size);
}

}
