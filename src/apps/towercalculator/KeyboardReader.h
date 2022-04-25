#ifndef KEYBOARDREADER_H
#define KEYBOARDREADER_H

#include "core/eventreceiver/ReadEventReceiver.h"

#include <functional>

class KeyboardReader : core::eventreceiver::ReadEventReceiver {
public:
    explicit KeyboardReader(const std::function<void(long)> cb);

private:
    void* operator new(std::size_t size) = delete;

protected:
    void readEvent() override;
    void unobservedEvent() override;

    std::function<void(long)> callBack;

    // void foo(long value);
};

#endif // KEYBOARDREADER_H
