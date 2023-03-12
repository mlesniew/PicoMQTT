#include "Arduino.h"

#include "client_wrapper.h"

#include "mqtt_debug.h"

namespace NanoMQTT {

ClientWrapper::ClientWrapper(::Client & client, unsigned long socket_timeout_seconds):
    socket_timeout_millis(socket_timeout_seconds * 1000), client(client), last_read(0), last_write(0) {
    TRACE_FUNCTION
}

int ClientWrapper::connect(IPAddress ip, uint16_t port) {
    TRACE_FUNCTION
    return client.connect(ip, port);
}

int ClientWrapper::connect(const char * host, uint16_t port) {
    TRACE_FUNCTION
    return client.connect(host, port);
}

void ClientWrapper::flush() {
    TRACE_FUNCTION
    client.flush();
}

void ClientWrapper::stop() {
    TRACE_FUNCTION
    client.stop();
}

uint8_t ClientWrapper::connected() {
    TRACE_FUNCTION
    return client.connected();
}

ClientWrapper::operator bool() {
    TRACE_FUNCTION
    return bool(client);
}

// reads
int ClientWrapper::available_wait(unsigned long timeout) {
    TRACE_FUNCTION
    const unsigned long start_millis = millis();
    while (true) {
        const int ret = client.available();
        if (ret > 0) {
            return ret;
        }
        if (!client.connected()) {
            // A disconnected client might still have unread data waiting in buffers.  Don't move this check earlier.
            return 0;
        }
        const unsigned long elapsed = millis() - start_millis;
        if (elapsed > timeout) {
            return 0;
        }
        yield();
    }
}

int ClientWrapper::available() {
    TRACE_FUNCTION
    return client.available();
}

int ClientWrapper::read(uint8_t * buf, size_t size) {
    TRACE_FUNCTION
    const unsigned long start_millis = millis();
    size_t ret = 0;

    while (ret < size) {
        const unsigned long now_millis = millis();
        const unsigned long elapsed_millis = now_millis - start_millis;

        if (elapsed_millis > socket_timeout_millis) {
            // timeout
            stop();
            return 0;
        }

        const unsigned long remaining_millis = socket_timeout_millis - elapsed_millis;

        const int available = available_wait(remaining_millis);
        if (!available) {
            // timeout
            stop();
            return 0;
        }

        const int bytes_to_read = size - ret < (size_t) available ? size - ret : (size_t) available;

        const int bytes_read = client.read(buf + ret, bytes_to_read);
        if (bytes_read <= 0) {
            // connection error
            stop();
            return 0;
        }

        last_read = millis();
        ret += bytes_read;
    }

    return ret;
}

int ClientWrapper::read() {
    TRACE_FUNCTION
    if (!available_wait(socket_timeout_millis)) {
        return -1;
    }
    return client.read();
}

int ClientWrapper::peek() {
    TRACE_FUNCTION
    if (!available_wait(socket_timeout_millis)) {
        return -1;
    }
    return client.peek();
}

// writes
size_t ClientWrapper::write(const uint8_t * buffer, size_t size) {
    TRACE_FUNCTION
    size_t ret = 0;

    while (client.connected() && ret < size) {
        const int bytes_written = client.write(buffer + ret, size - ret);
        if (bytes_written <= 0) {
            // connection error
            stop();
            return 0;
        }

        last_write = millis();
        ret += bytes_written;
    }

    return ret;
}

size_t ClientWrapper::write(uint8_t value) {
    TRACE_FUNCTION
    return write(&value, 1);
}

unsigned long ClientWrapper::get_millis_since_last_read() const {
    TRACE_FUNCTION
    return millis() - last_read;
}

unsigned long ClientWrapper::get_millis_since_last_write() const {
    TRACE_FUNCTION
    return millis() - last_write;
}

}
