#pragma once

#include <memory>

#include "Print.h"
#include "Stream.h"

namespace PicoMQTT {

class Buffer: public Print {
    public:
        struct Allocation {
            char * buffer;
            size_t size;
        };

        Buffer(const size_t size);
        bool is_overflown() const;
        void reset();

        virtual size_t write(uint8_t value) override;
        virtual size_t write(const uint8_t * buffer, size_t size) override;
        virtual int availableForWrite() override;

        const char * read_string(Stream & stream, size_t size);
        Allocation read(Stream & stream, size_t size);
        size_t get_remaining_size() const;
        const size_t size;

        Allocation allocate(size_t size);
        Allocation get_content();

    protected:
        size_t pos;
        bool overflow;

        const std::unique_ptr<char[]> buffer;
};

}
