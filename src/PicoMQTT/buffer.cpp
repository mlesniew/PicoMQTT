#include "buffer.h"
#include "debug.h"

namespace PicoMQTT {

Buffer::Buffer(const size_t size): size(size), pos(0), overflow(false), buffer(new char[size]) {
    TRACE_FUNCTION
}

bool Buffer::is_overflown() const {
    TRACE_FUNCTION
    return overflow;
}

void Buffer::reset() {
    TRACE_FUNCTION
    pos = 0;
    overflow = false;
}

size_t Buffer::write(uint8_t value) {
    TRACE_FUNCTION
    return Buffer::write((const uint8_t *) &value, 1);
}

size_t Buffer::write(const uint8_t * buffer, size_t size) {
    TRACE_FUNCTION
    auto alloc = allocate(size);
    memcpy(alloc.buffer, buffer, alloc.size);
    return alloc.size;
}

int Buffer::availableForWrite() {
    TRACE_FUNCTION
    return get_remaining_size() > 0;
}

const char * Buffer::read_string(Stream & stream, size_t size) {
    TRACE_FUNCTION
    auto alloc = allocate(size + 1);
    if (alloc.size == 0) {
        return "";
    }
    stream.read(alloc.buffer, alloc.size - 1);
    alloc.buffer[alloc.size - 1] = '\0';
    return alloc.buffer;
}

Buffer::Allocation Buffer::read(Stream & stream, size_t size) {
    TRACE_FUNCTION
    auto alloc = allocate(size);
    stream.read(alloc.buffer, alloc.size);
    return alloc;
}

size_t Buffer::get_remaining_size() const {
    TRACE_FUNCTION
    return size - pos;
}

Buffer::Allocation Buffer::get_content() {
    TRACE_FUNCTION
    return { buffer.get(), pos };
}

Buffer::Allocation Buffer::allocate(size_t size) {
    TRACE_FUNCTION
    const size_t remaining = get_remaining_size();
    Allocation ret = {
        buffer.get() + pos,
        size
    };

    if (size > remaining) {
        overflow = true;
        ret.size = remaining;
    }

    pos += ret.size;
    return ret;
}

}
