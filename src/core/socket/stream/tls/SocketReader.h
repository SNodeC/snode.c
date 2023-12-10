/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETREADER_H
#define CORE_SOCKET_STREAM_TLS_SOCKETREADER_H

#include "core/socket/stream/SocketReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdio>
#include <functional>
#include <openssl/types.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    class SocketReader : public core::socket::stream::SocketReader {
    private:
        using Super = core::socket::stream::SocketReader;
        using Super::Super;

        ssize_t read(char* junk, std::size_t junkLen) override;

    protected:
        virtual void doSSLShutdown() = 0;

        virtual void doSSLHandshake(const std::function<void()>& onSuccess,
                                    const std::function<void()>& onTimeout,
                                    const std::function<void(int)>& onStatus) = 0;

        SSL* ssl = nullptr;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETREADER_H
