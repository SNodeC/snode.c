/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketConnectionBase.h"

namespace net::socket::tcp {

    template <typename SocketReaderT, typename SocketWriterT>
    class SocketConnection
        : public SocketConnectionBase
        , public SocketReaderT
        , public SocketWriterT {
    public:
        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;
        using Socket = typename SocketReader::Socket;

        void* operator new(size_t size) {
            SocketConnection<SocketReader, SocketWriter>::lastAllocAddress = malloc(size);

            return SocketConnection<SocketReader, SocketWriter>::lastAllocAddress;
        }

        void operator delete(void* socketConnection_v) {
            free(socketConnection_v);
        }

        SocketConnection() = delete;

        SocketConnection(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                         const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                         const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                         const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                         const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                         const std::function<void(SocketConnection* socketConnection)>& onDisconnect)
            : SocketReader(
                  [this, onRead](const char* junk, ssize_t junkLen) -> void {
                      onRead(this, junk, junkLen);
                  },
                  [this, onReadError](int errnum) -> void {
                      onReadError(this, errnum);
                  })
            , SocketWriter([this, onWriteError](int errnum) -> void {
                onWriteError(this, errnum);
            })
            , onDestruct(onDestruct)
            , onDisconnect(onDisconnect)
            , isDynamic(this == lastAllocAddress) {
            onConstruct(this);
        }

        ~SocketConnection() override {
            onDestruct(this);
        }

    private:
        void unobserved() override {
            onDisconnect(this);

            if (isDynamic) {
                delete this;
            }
        }

    public:
        void enqueue(const char* junk, size_t junkLen) override {
            SocketWriter::enqueue(junk, junkLen);
        }

        void enqueue(const std::string& data) override {
            enqueue(data.c_str(), data.size());
        }

        void end(bool instantly = false) override {
            SocketReader::disable();
            if (instantly) {
                SocketWriter::disable();
            }
        }

        const typename Socket::SocketAddress& getRemoteAddress() const {
            return remoteAddress;
        }

        void setRemoteAddress(const typename Socket::SocketAddress& remoteAddress) {
            this->remoteAddress = remoteAddress;
        }

    private:
        typename Socket::SocketAddress remoteAddress{};
        std::function<void(SocketConnection* socketConnection)> onDestruct;
        std::function<void(SocketConnection* socketConnection)> onDisconnect;

        bool isDynamic;
        static void* lastAllocAddress;
    };

    template <typename SocketReader, typename SocketWriter>
    void* SocketConnection<SocketReader, SocketWriter>::lastAllocAddress = nullptr;

} // namespace net::socket::tcp

#endif // SOCKETCONNECTION_H
