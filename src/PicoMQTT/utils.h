#pragma once

#include <utility>

namespace PicoMQTT {

template <typename T>
struct SocketOwner {
    SocketOwner() {}
    template <typename ...Args>
    SocketOwner(Args && ...args): socket(std::forward<Args>(args)...) {}
    virtual ~SocketOwner() {}
    T socket;
};

}
