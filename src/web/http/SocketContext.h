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

#ifndef WEB_HTTP_SOCKETCONTEXT_H
#define WEB_HTTP_SOCKETCONTEXT_H

#include "core/socket/stream/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    class SocketContext : public core::socket::stream::SocketContext {
    public:
        SocketContext(const SocketContext&) = delete;

        SocketContext& operator=(const SocketContext&) = delete;

    private:
        using Super = core::socket::stream::SocketContext;
        using Super::Super;

    protected:
        std::size_t onReceiveFromPeer() override = 0;

    public:
        virtual void sendToPeerCompleted() = 0;
    };

} // namespace web::http

#endif // WEB_HTTP_SOCKETCONTEXT_H
