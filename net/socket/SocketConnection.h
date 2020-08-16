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

#ifndef SOCKETCONNECTIONBASE_H
#define SOCKETCONNECTIONBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "InetAddress.h"
#include "SocketConnectionBase.h"

template <typename SocketReader, typename SocketWriter>
class SocketConnection
    : public SocketConnectionBase
    , public SocketReader
    , public SocketWriter {
public:
    // NOLINT(cppcoreguidelines-pro-type-member-init)
    SocketConnection() = delete;

    void* operator new(size_t size) {
        SocketConnection* sc = reinterpret_cast<SocketConnection*>(malloc(size));
        sc->isDynamic = true;

        return sc;
    }

    void operator delete(void* sc_v) {
        SocketConnection* sc = reinterpret_cast<SocketConnection*>(sc_v);
        if (sc->isDynamic) {
            free(sc_v);
        }
    }

protected:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
    SocketConnection(int csFd, const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                     const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* cs, int errnum)>& onWriteError,
                     const std::function<void(SocketConnection* cs)>& onDisconnect)
        : SocketReader(
              [&](const char* junk, ssize_t n) -> void {
                  onRead(this, junk, n);
              },
              [&](int errnum) -> void {
                  onReadError(this, errnum);
              })
        , SocketWriter([&](int errnum) -> void {
            onWriteError(this, errnum);
        })
        , onDisconnect(onDisconnect) {
        this->attachFd(csFd);
    }

public:
    static SocketConnection* create(int csFd, const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                                    const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                                    const std::function<void(SocketConnection* cs, int errnum)>& onWriteError,
                                    const std::function<void(SocketConnection* cs)>& onDisconnect) {
        return new SocketConnection(csFd, onRead, onReadError, onWriteError, onDisconnect);
    } // NOLINT(cppcoreguidelines-pro-type-member-init)

    static SocketConnection* create(const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                                    const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                                    const std::function<void(SocketConnection* cs, int errnum)>& onWriteError,
                                    const std::function<void(SocketConnection* cs)>& onDisconnect) {
        return SocketConnection::create(0, onRead, onReadError, onWriteError, onDisconnect);
    } // NOLINT(cppcoreguidelines-pro-type-member-init)

private:
    void unobserved() override {
        onDisconnect(this);

        if (this->isDynamic) {
            delete this;
        }
    }

public:
    void enqueue(const char* buffer, int size) override {
        this->SocketWriter::enqueue(buffer, size);
    }

    void enqueue(const std::string& data) override {
        this->enqueue(data.c_str(), data.size());
    }

    void end(bool instantly = false) override {
        SocketReader::disable();
        if (instantly) {
            SocketWriter::disable();
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
    std::function<void(SocketConnection* cs)> onDisconnect;
    bool isDynamic;

public:
    using ReaderType = SocketReader;
    using WriterType = SocketWriter;
};

#endif // SOCKETCONNECTIONBASE_H
