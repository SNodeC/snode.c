// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/socket/stream/stdio/SocketReader.h"

#include "core/system/unistd.h"

namespace core::socket::stream::stdio {

    SocketReader::~SocketReader() {
    }

    ssize_t SocketReader::read(char* chunk, std::size_t chunkLen) {
        return core::system::read(getRegisteredFd(), chunk, chunkLen);
    }

} // namespace core::socket::stream::stdio
