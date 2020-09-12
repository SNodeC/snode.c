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

#ifndef TLS_SOCKETWRITER_H
#define TLS_SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>     // for size_t
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketWriter.h"
#include "socket/tls/Socket.h" // IWYU pragma: keep

// IWYU pragma: no_forward_declare tls::Socket

namespace net::socket::tls {

    class SocketWriter : public socket::SocketWriter<tls::Socket> {
    protected:
        using socket::SocketWriter<tls::Socket>::SocketWriter;

        ssize_t write(const char* junk, size_t junkLen) override;
    };

}; // namespace net::socket::tls

#endif // TLS_SOCKETWRITER_H
