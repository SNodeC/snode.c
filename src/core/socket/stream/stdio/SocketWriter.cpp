// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/socket/stream/stdio/SocketWriter.h"

#include "core/system/unistd.h"

namespace core::socket::stream::stdio {

    SocketWriter::~SocketWriter() {
    }

    ssize_t SocketWriter::write(const char* chunk, std::size_t chunkLen) {
        return core::system::write(getRegisteredFd(), chunk, chunkLen);
    }

} // namespace core::socket::stream::stdio
