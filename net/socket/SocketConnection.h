/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "InetAddress.h"
#include "SocketConnectionBase.h"

namespace net::socket {

    template <typename SocketReaderT, typename SocketWriterT>
    class SocketConnection
        : public SocketConnectionBase
        , public SocketReaderT
        , public SocketWriterT {
    public:
        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;

        void* operator new(size_t size) {
            SocketConnection<SocketReader, SocketWriter>::lastAllocAddress = malloc(size);

            return SocketConnection<SocketReader, SocketWriter>::lastAllocAddress;
        }

        void operator delete(void* socketConnection_v) {
            free(socketConnection_v);
        }

        // NOLINT(cppcoreguidelines-pro-type-member-init)
        SocketConnection() = delete;

    protected:
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
        SocketConnection(const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
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
            , onDisconnect(onDisconnect)
            , isDynamic(this == lastAllocAddress) {
        }

    public:
        static SocketConnection*
        create(const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
               const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
               const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
               const std::function<void(SocketConnection* socketConnection)>& onDisconnect) {
            return new SocketConnection(onRead, onReadError, onWriteError, onDisconnect);
        } // NOLINT(cppcoreguidelines-pro-type-member-init)

    private:
        void unobserved() override {
            onDisconnect(this);

            if (isDynamic) {
                delete this;
            }
        }

    public:
        void enqueue(const char* junk, size_t junkLen) override {
            SocketWriterT::enqueue(junk, junkLen);
        }

        void enqueue(const std::string& data) override {
            enqueue(data.c_str(), data.size());
        }

        void end(bool instantly = false) override {
            SocketReaderT::disable();
            if (instantly) {
                SocketWriterT::disable();
            }
        }

        InetAddress& getRemoteAddress() {
            return remoteAddress;
        }

        void setRemoteAddress(const InetAddress& remoteAddress) {
            this->remoteAddress = remoteAddress;
        }

    private:
        InetAddress remoteAddress{};
        std::function<void(SocketConnection* socketConnection)> onDisconnect;

        bool isDynamic;

        static void* lastAllocAddress;
    };

    template <typename SocketReaderT, typename SocketWriterT>
    void* SocketConnection<SocketReaderT, SocketWriterT>::lastAllocAddress = nullptr;

} // namespace net::socket

#endif // SOCKETCONNECTION_H
