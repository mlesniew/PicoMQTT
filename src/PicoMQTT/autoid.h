#pragma once

namespace PicoMQTT {

class AutoId {
    public:
        typedef unsigned int Id;

        AutoId(): id(generate_id()) {}
        AutoId(const AutoId &) = default;

        const Id id;

    private:
        static Id generate_id() {
            static Id next_id = 1;
            return next_id++;
        }
};

}
