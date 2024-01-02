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

#include "web/http/server/RequestContextBase.h"

#include "web/http/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    web::http::server::RequestContextBase::RequestContextBase(web::http::SocketContext* socketContext)
        : socketContext(socketContext) {
    }

    RequestContextBase::~RequestContextBase() {
    }

    void RequestContextBase::socketContextGone() {
        socketContext = nullptr;
    }

    core::socket::SocketContext*
    RequestContextBase::switchSocketContext(core::socket::stream::SocketContextFactory* socketContextUpgradeFactory) {
        core::socket::SocketContext* newSocketContext = nullptr;
        if (socketContext != nullptr) {
            newSocketContext = socketContext->switchSocketContext(socketContextUpgradeFactory);
        } else {
            delete this;
        }

        return newSocketContext;
    }

    void RequestContextBase::sendToPeer(const char* junk, std::size_t junkLen) {
        if (socketContext != nullptr) {
            socketContext->sendToPeer(junk, junkLen);
        }
    }

    void RequestContextBase::sendToPeerCompleted() {
        if (socketContext != nullptr) {
            socketContext->sendToPeerCompleted();
        } else {
            delete this;
        }
    }

    void RequestContextBase::close() {
        if (socketContext != nullptr) {
            socketContext->close();
        } else {
            delete this;
        }
    }

} // namespace web::http::server
