#ifndef WSTRANSCEIVER_H
#define WSTRANSCEIVER_H

#include "WSReceiver.h"
#include "WSTransmitter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    class WSTransCeiver
        : public WSReceiver
        , public WSTransmitter {
    public:
        WSTransCeiver();

        void onMessageStart(int opCode) override;
        void onMessageData(char* junk, uint64_t junkLen) override;
        void onMessageEnd() override;

        void onFrameReady(char* frame, uint64_t frameLength) override;
    };

} // namespace http::websocket

#endif // WSTRANSCEIVER_H
