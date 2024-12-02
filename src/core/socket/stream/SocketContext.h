/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef CORE_SOCKET_STREAM_SOCKETCONTEXT_H
#define CORE_SOCKET_STREAM_SOCKETCONTEXT_H

#include "core/socket/SocketContext.h"

namespace core::pipe {
    class Source;
}

namespace core::socket::stream {
    class SocketConnection;

} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketContext : public core::socket::SocketContext {
    private:
        using Super = core::socket::SocketContext;

    public:
        explicit SocketContext(core::socket::stream::SocketConnection* socketConnection);

        using Super::sendToPeer;

        void sendToPeer(const char* chunk, std::size_t chunkLen) const final;
        bool streamToPeer(core::pipe::Source* source) const;
        void streamEof();

        std::size_t readFromPeer(char* chunk, std::size_t chunklen) const final;

        void setTimeout(const utils::Timeval& timeout) final;

        void shutdownRead();
        void shutdownWrite(bool forceClose = false);
        void close() override;

        SocketConnection* getSocketConnection() const;
        virtual void switchSocketContext(SocketContext* newSocketContext);

    protected:
        void onWriteError(int errnum) override;
        void onReadError(int errnum) override;

    private:
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;

        core::socket::stream::SocketConnection* socketConnection;

        template <typename PhysicalSocketT, class SocketReaderT, class SocketWriterT>
        friend class SocketConnectionT;
        friend class SocketConnection;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONTEXT_H
