#include "client.h"
#include "debug.h"

namespace PicoMQTT {

BasicClient::BasicClient(unsigned long keep_alive_seconds, unsigned long socket_timeout_seconds)
    : Connection(keep_alive_seconds, socket_timeout_seconds) {
    TRACE_FUNCTION
}

BasicClient::BasicClient(const ::WiFiClient & client, unsigned long keep_alive_seconds,
                         unsigned long socket_timeout_seconds)
    : Connection(client, keep_alive_seconds, socket_timeout_seconds) {
    TRACE_FUNCTION
}

bool BasicClient::connect(
    const char * host,
    uint16_t port,
    const char * id,
    const char * user,
    const char * pass,
    const char * will_topic,
    const char * will_message,
    const size_t will_message_length,
    uint8_t will_qos,
    bool will_retain,
    const bool clean_session,
    ConnectReturnCode * connect_return_code) {
    TRACE_FUNCTION

    if (connect_return_code) {
        *connect_return_code = CRC_UNDEFINED;
    }

    client.stop();

    if (!client.connect(host, port)) {
        return false;
    }

    message_id_generator.reset();

    const bool will = will_topic && will_message;

    const uint8_t connect_flags =
        (user ? 1 : 0) << 7
        | (user && pass ? 1 : 0) << 6
        | (will && will_retain ? 1 : 0) << 5
        | (will && will_qos ? 1 : 0) << 3
        | (will ? 1 : 0) << 2
        | (clean_session ? 1 : 0) << 1;

    const size_t client_id_length = strlen(id);
    const size_t will_topic_length = (will && will_topic) ? strlen(will_topic) : 0;
    const size_t user_length = user ? strlen(user) : 0;
    const size_t pass_length = pass ? strlen(pass) : 0;

    const size_t total_size = 6     // protocol name
                              + 1                         // protocol level
                              + 1                         // connect flags
                              + 2                         // keep-alive
                              + client_id_length + 2
                              + (will ? will_topic_length + 2 : 0)
                              + (will ? will_message_length + 2 : 0)
                              + (user ? user_length + 2 : 0)
                              + (user && pass ? pass_length + 2 : 0);

    auto packet = build_packet(Packet::CONNECT, 0, total_size);
    packet.write_string("MQTT", 4);
    packet.write_u8(4);
    packet.write_u8(connect_flags);
    packet.write_u16(keep_alive_millis / 1000);
    packet.write_string(id, client_id_length);

    if (will) {
        packet.write_string(will_topic, will_topic_length);
        packet.write_string(will_message, will_message_length);
    }

    if (user) {
        packet.write_string(user, user_length);
        if (pass)  {
            packet.write_string(pass, pass_length);
        }
    }

    if (!packet.send()) {
        return false;
    }

    wait_for_reply(Packet::CONNACK, [this, connect_return_code](IncomingPacket & packet) {
        TRACE_FUNCTION
        if (packet.size != 2) {
            on_protocol_violation();
            return;
        }

        /* const uint8_t connect_ack_flags = */ packet.read_u8();
        const uint8_t crc = packet.read_u8();

        if (connect_return_code) {
            *connect_return_code = (ConnectReturnCode) crc;
        }

        if (crc != 0) {
            // connection refused
            client.stop();
        }
    });

    return client.connected();
}

void BasicClient::loop() {
    TRACE_FUNCTION

    if (client.connected() && get_millis_since_last_write() >= keep_alive_millis) {
        // ping time!
        build_packet(Packet::PINGREQ).send();
        wait_for_reply(Packet::PINGRESP, [](IncomingPacket &) {});
    }

    Connection::loop();
}

Publisher::Publish BasicClient::begin_publish(const char * topic, const size_t payload_size,
        uint8_t qos, bool retain, uint16_t message_id) {
    TRACE_FUNCTION
    return Publish(
               *this,
               client.status() ? client : PrintMux(),
               topic, payload_size,
               (qos >= 1) ? 1 : 0,
               retain,
               message_id,  // dup if message_id is non-zero
               message_id ? message_id : message_id_generator.generate()  // generate only if message_id == 0
           );
}

bool BasicClient::on_publish_complete(const Publish & publish) {
    TRACE_FUNCTION
    if (publish.qos == 0) {
        return true;
    }

    bool confirmed = false;
    wait_for_reply(Packet::PUBACK, [&publish, &confirmed](IncomingPacket & puback) {
        confirmed |= (puback.read_u16() == publish.message_id);
    });

    return confirmed;
}

bool BasicClient::subscribe(const String & topic, uint8_t qos, uint8_t * qos_granted) {
    TRACE_FUNCTION
    if (qos > 1) {
        return false;
    }

    const size_t topic_size = topic.length();
    const uint16_t message_id = message_id_generator.generate();

    auto packet = build_packet(Packet::SUBSCRIBE, 0b0010, 2 + 2 + topic_size + 1);
    packet.write_u16(message_id);
    packet.write_string(topic.c_str(), topic_size);
    packet.write_u8(qos);
    packet.send();

    uint8_t code = 0x80;

    wait_for_reply(Packet::SUBACK, [this, message_id, &code](IncomingPacket & packet) {
        if (packet.read_u16() != message_id) {
            on_protocol_violation();
        } else {
            code = packet.read_u8();
        }
    });

    if (code == 0x80) {
        return false;
    }

    if (qos_granted) {
        *qos_granted = code;
    }

    return client.connected();
}

bool BasicClient::unsubscribe(const String & topic) {
    TRACE_FUNCTION

    const size_t topic_size = topic.length();
    const uint16_t message_id = message_id_generator.generate();

    auto packet = build_packet(Packet::UNSUBSCRIBE, 0b0010, 2 + 2 + topic_size);
    packet.write_u16(message_id);
    packet.write_string(topic.c_str(), topic_size);
    packet.send();

    wait_for_reply(Packet::UNSUBACK, [this, message_id](IncomingPacket & packet) {
        if (packet.read_u16() != message_id) {
            on_protocol_violation();
        }
    });

    return client.connected();
}

Client::Client(const char * host, uint16_t port, const char * id, const char * user, const char * password,
               unsigned long reconnect_interval_millis)
    : host(host), port(port), client_id(id), username(user), password(password),
      will({"", "", 0, false}),
reconnect_interval_millis(reconnect_interval_millis),
last_reconnect_attempt(millis() - reconnect_interval_millis) {
    TRACE_FUNCTION
}

Client::SubscriptionId Client::subscribe(const String & topic_filter, MessageCallback callback) {
    TRACE_FUNCTION
    BasicClient::subscribe(topic_filter);
    return SubscribedMessageListener::subscribe(topic_filter, callback);
}

void Client::unsubscribe(const String & topic_filter) {
    TRACE_FUNCTION
    BasicClient::unsubscribe(topic_filter);
    SubscribedMessageListener::unsubscribe(topic_filter);
}

void Client::on_message(const char * topic, IncomingPacket & packet) {
    SubscribedMessageListener::fire_message_callbacks(topic, packet);
}

void Client::loop() {
    TRACE_FUNCTION
    if (!client.connected()) {
        if (host.isEmpty() || !port) {
            return;
        }

        if (millis() - last_reconnect_attempt < reconnect_interval_millis) {
            return;
        }

        const bool connection_established = connect(host.c_str(), port,
                                            client_id.isEmpty() ? "" : client_id.c_str(),
                                            username.isEmpty() ? nullptr : username.c_str(),
                                            password.isEmpty() ? nullptr : password.c_str(),
                                            will.topic.isEmpty() ? nullptr : will.topic.c_str(),
                                            will.payload.isEmpty() ? nullptr : will.payload.c_str(),
                                            will.payload.isEmpty() ? 0 : will.payload.length(),
                                            will.qos, will.retain);

        last_reconnect_attempt = millis();

        if (!connection_established) {
            return;
        }

        for (const auto & kv : subscriptions) {
            BasicClient::subscribe(kv.first.c_str());
        }

        on_connect();
    }

    BasicClient::loop();
}

void Client::on_connect() {
    TRACE_FUNCTION
    BasicClient::on_connect();
    if (connected_callback) {
        connected_callback();
    }
}

void Client::on_disconnect() {
    TRACE_FUNCTION
    BasicClient::on_disconnect();
    if (disconnected_callback) {
        connected_callback();
    }
}

}
