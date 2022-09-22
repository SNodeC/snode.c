#include "KeyboardReader.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

KeyboardReader::KeyboardReader(const std::function<void(long)> cb)
    : core::eventreceiver::ReadEventReceiver("KeyboardReader")
    , callBack(cb) {
    enable(STDIN_FILENO);
}

void KeyboardReader::readEvent() {
    std::cout << "ReadEvent" << std::endl;
    /*
        long value;

        std::cin >> long;
    */

    char buffer[256];
    ssize_t ret = read(STDIN_FILENO, buffer, 256);

    if (ret > 0) {
        buffer[ret] = 0;
        try {
            long value = 0;
            value = std::stol(buffer);

            std::cout << "Value = " << value << std::endl;

            callBack(value);
        } catch (std::invalid_argument& e) {
            std::cout << e.what() << ": "
                      << "no conversion possible: input = " << buffer << std::endl;
        }
    }
}

void KeyboardReader::unobservedEvent() {
    //    delete this; // do not delete
}
