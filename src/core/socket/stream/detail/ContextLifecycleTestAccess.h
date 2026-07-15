#ifndef CORE_SOCKET_STREAM_DETAIL_CONTEXTLIFECYCLETESTACCESS_H
#define CORE_SOCKET_STREAM_DETAIL_CONTEXTLIFECYCLETESTACCESS_H

#include "core/socket/stream/SocketContext.h"

namespace core::socket::stream::detail {

    struct ContextLifecycleTestAccess {
        static void attach(SocketContext* context) {
            context->attach();
        }

        static void detachForContextSwitch(SocketContext* context) {
            context->detach(SocketContext::DetachReason::ContextSwitch);
        }

        static void detachForConnectionClose(SocketContext* context) {
            context->detach(SocketContext::DetachReason::ConnectionClose);
        }
    };

} // namespace core::socket::stream::detail

#endif // CORE_SOCKET_STREAM_DETAIL_CONTEXTLIFECYCLETESTACCESS_H
