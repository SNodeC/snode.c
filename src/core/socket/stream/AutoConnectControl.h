/*
 * Backward compatibility header.
 */

#ifndef CORE_SOCKET_STREAM_AUTOCONNECTCONTROL_H
#define CORE_SOCKET_STREAM_AUTOCONNECTCONTROL_H

#include "core/socket/stream/FlowController.h"

namespace core::socket::stream {

    template <typename FlowControllerT>
    class AutoConnectControl : public FlowControllerT {
    public:
        using FlowControllerT::FlowControllerT;

        void stopReconnectAndRetry() {
            this->stopReconnect();
            this->stopRetry();
        }
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_AUTOCONNECTCONTROL_H
