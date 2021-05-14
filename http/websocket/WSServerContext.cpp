#include "http/websocket/WSServerContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    void WSServerContext::receiveData(const char* junk, std::size_t junklen) {
        receive(const_cast<char*>(junk), junklen);
    }

    void WSServerContext::onReadError([[maybe_unused]] int errnum) {
    }

    void WSServerContext::onWriteError([[maybe_unused]] int errnum) {
    }

    void WSServerContext::onMessageStart(int opCode) {
        std::cout << "Message Start - OpCode: " << opCode << std::endl;
    }

    void WSServerContext::onMessageData(char* junk, uint64_t junkLen) {
        std::cout << std::string(junk, static_cast<std::size_t>(junkLen));
    }

    void WSServerContext::onMessageEnd() {
        std::cout << std::endl << "Message End" << std::endl;
    }

    void WSServerContext::onFrameReady(char* frame, uint64_t frameLength) {
        receive(frame, static_cast<std::size_t>(frameLength));
    }

} // namespace http::websocket
