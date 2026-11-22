// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef CORE_SOCKET_STREAM_STDIO_SOCKETWRITER_H
#define CORE_SOCKET_STREAM_STDIO_SOCKETWRITER_H

#include "core/socket/stream/SocketWriter.h"
#include "core/system/unistd.h"

namespace core::socket::stream::stdio {

    /// Buffered descriptor output; reuses the original stream queue and backpressure policy.
    class SocketWriter : public core::socket::stream::SocketWriter {
    private:
        using Super = core::socket::stream::SocketWriter;
        using Super::Super;

    public:
        /// Destroy after publisher removal on the event-loop thread.
        ~SocketWriter() override;

    protected:
        /// Write only through event-loop dispatch to the registered output handle.
        ssize_t write(const char* chunk, std::size_t chunkLen) override;
    };

} // namespace core::socket::stream::stdio

#endif // CORE_SOCKET_STREAM_STDIO_SOCKETWRITER_H
