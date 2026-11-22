/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CORE_SOCKET_STREAM_STDIO_SOCKETCONNECTION_H
#define CORE_SOCKET_STREAM_STDIO_SOCKETCONNECTION_H

#include "core/socket/stream/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/stdio/SocketReader.h"
#include "core/socket/stream/stdio/SocketWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::stdio {

    /// Original stream connection engine composed with descriptor read/write hooks.
    template <typename PhysicalSocket, typename Config>
    class SocketConnection final : public core::socket::stream::SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config> {
    private:
        using Super = core::socket::stream::SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>;

    public:
        /// Construct only; call activate once and never use the pointer after false.
        SocketConnection(PhysicalSocket&& physicalSocket,
                         const std::function<void(SocketConnection*)>& onDisconnect,
                         std::uint64_t connectionId,
                         const std::shared_ptr<Config>& config);
        /// Attach the original factory/context after successful activation; original connection owns the result.
        using Super::setSocketContext;

        /// Inspect establishment on this loop turn; a close request immediately makes this false.
        bool isActive() const {
            return stdio::SocketReader::isEnabled() && stdio::SocketWriter::isEnabled();
        }
    };

} // namespace core::socket::stream::stdio

#endif // CORE_SOCKET_STREAM_STDIO_SOCKETCONNECTION_H
