#include "Arduino.h"

#include "client_wrapper.h"

#include "debug.h"

namespace PicoMQTT {

ClientWrapper::ClientWrapper(::Client & client, unsigned long socket_timeout_millis):
    socket_timeout_millis(socket_timeout_millis), client(client) {
    TRACE_FUNCTION
}

// reads
int ClientWrapper::available_wait(unsigned long timeout) {
    TRACE_FUNCTION
    const unsigned long start_millis = millis();
    while (true) {
        const int ret = available();
        if (ret > 0) {
            return ret;
        }
        if (!connected()) {
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

int ClientWrapper::read(uint8_t * buf, size_t size) {
    TRACE_FUNCTION
    const unsigned long start_millis = millis();
    size_t ret = 0;

    while (ret < size) {
        const unsigned long now_millis = millis();
        const unsigned long elapsed_millis = now_millis - start_millis;

        if (elapsed_millis > socket_timeout_millis) {
            // timeout
            abort();
            break;
        }

        const unsigned long remaining_millis = socket_timeout_millis - elapsed_millis;

        const int available_size = available_wait(remaining_millis);
        if (available_size <= 0) {
            // timeout
            abort();
            break;
        }

        const int chunk_size = size - ret < (size_t) available_size ? size - ret : (size_t) available_size;

        const int bytes_read = client.read(buf + ret, chunk_size);
        if (bytes_read <= 0) {
            // connection error
            abort();
            break;
        }

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

    while (connected() && ret < size) {
        const int bytes_written = client.write(buffer + ret, size - ret);
        if (bytes_written <= 0) {
            // connection error
            abort();
            return 0;
        }

        ret += bytes_written;
    }

    return ret;
}

size_t ClientWrapper::write(uint8_t value) {
    TRACE_FUNCTION
    return write(&value, 1);
}

// simple wrappers forwarding requests to this->client
int ClientWrapper::connect(IPAddress ip, uint16_t port) {
    TRACE_FUNCTION
    return client.connect(ip, port);
}

int ClientWrapper::connect(const char * host, uint16_t port) {
    TRACE_FUNCTION
    return client.connect(host, port);
}

#ifdef PICOMQTT_EXTRA_CONNECT_METHODS

int ClientWrapper::connect(IPAddress ip, uint16_t port, int32_t timeout) {
    TRACE_FUNCTION
    return client.connect(ip, port, timeout);
}

int ClientWrapper::connect(const char * host, uint16_t port, int32_t timeout) {
    TRACE_FUNCTION
    return client.connect(host, port, timeout);
}

#endif

int ClientWrapper::available() {
    TRACE_FUNCTION
    return client.available();
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
    return bool(client);
}

}
