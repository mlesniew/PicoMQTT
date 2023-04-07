#pragma once

namespace PicoMQTT {

class PicoMQTTInterface {
    public:
        virtual ~PicoMQTTInterface() {}
        virtual void begin() {}
        virtual void stop() {}
        virtual void loop() {}
};

}
