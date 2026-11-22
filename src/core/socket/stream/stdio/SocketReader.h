// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef CORE_SOCKET_STREAM_STDIO_SOCKETREADER_H
#define CORE_SOCKET_STREAM_STDIO_SOCKETREADER_H

#include "core/socket/stream/SocketReader.h"
#include "core/system/unistd.h"

namespace core::socket::stream::stdio {

    /// Buffered descriptor input; construction and lifetime belong to the original stream connection.
    class SocketReader : public core::socket::stream::SocketReader {
    private:
        using Super = core::socket::stream::SocketReader;
        using Super::Super;

    public:
        /// Destroy after publisher removal on the event-loop thread.
        ~SocketReader() override;

    protected:
        /// Read only through event-loop dispatch from the registered input handle.
        ssize_t read(char* chunk, std::size_t chunkLen) override;
    };

} // namespace core::socket::stream::stdio

#endif // CORE_SOCKET_STREAM_STDIO_SOCKETREADER_H
