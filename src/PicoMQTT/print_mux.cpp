#include "print_mux.h"
#include "debug.h"

namespace PicoMQTT {

size_t PrintMux::write(uint8_t value) {
    TRACE_FUNCTION
    for (auto print_ptr : prints) {
        print_ptr->write(value);
    }
    return 1;
}

size_t PrintMux::write(const uint8_t * buffer, size_t size) {
    TRACE_FUNCTION
    for (auto print_ptr : prints) {
        print_ptr->write(buffer, size);
    }
    return size;
}

void PrintMux::flush() {
    TRACE_FUNCTION
    for (auto print_ptr : prints) {
        print_ptr->flush();
    }
}

}
