#include "WSTransCeiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    WSTransCeiver::WSTransCeiver() {
    }

    void WSTransCeiver::onMessageStart(int opCode) {
        std::cout << "Message Start - OpCode: " << opCode << std::endl;
    }

    void WSTransCeiver::onMessageData(char* junk, uint64_t junkLen) {
        std::cout << std::string(junk, static_cast<std::size_t>(junkLen));
    }

    void WSTransCeiver::onMessageEnd() {
        std::cout << std::endl << "Message End" << std::endl;
    }

    void WSTransCeiver::onFrameReady(char* frame, uint64_t frameLength) {
        receive(frame, frameLength);
    }

} // namespace http::websocket
