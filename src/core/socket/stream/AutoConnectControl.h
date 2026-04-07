/*
 * Backward compatibility header.
 */

#ifndef CORE_SOCKET_STREAM_AUTOCONNECTCONTROL_H
#define CORE_SOCKET_STREAM_AUTOCONNECTCONTROL_H

#include "core/socket/stream/FlowController.h"

namespace core::socket::stream {

    class AutoConnectControl : public ClientFlowController {
    public:
        void stopReconnectAndRetry() {
            stopReconnect();
            stopRetry();
        }
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_AUTOCONNECTCONTROL_H
