#ifndef WSTRANSCEIVER_H
#define WSTRANSCEIVER_H

#include "http/server/ServerContextBase.h"
#include "http/websocket/WSReceiver.h"
#include "http/websocket/WSTransmitter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    class WSServerContext
        : public http::server::ServerContextBase
        , public WSReceiver
        , public WSTransmitter {
    public:
        void receiveData(const char* junk, std::size_t junklen) override;
        void onReadError(int errnum) override;
        void onWriteError(int errnum) override;

        void onMessageStart(int opCode) override;
        void onMessageData(char* junk, uint64_t junkLen) override;
        void onMessageEnd() override;

        void onFrameReady(char* frame, uint64_t frameLength) override;
    };

} // namespace http::websocket

#endif // WSTRANSCEIVER_H
