/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef APPS_MODEL_ECHOSOCKETCONTEXT_H
#define APPS_MODEL_ECHOSOCKETCONTEXT_H

#include "core/socket/stream/SocketContext.h"        // for SocketP...
#include "core/socket/stream/SocketContextFactory.h" // for SocketP...
#include "log/Logger.h"                              // for Writer

namespace apps::model {

    class EchoSocketContext : public core::socket::stream::SocketContext {
    public:
        explicit EchoSocketContext(core::socket::stream::SocketConnection* socketConnection)
            : core::socket::stream::SocketContext(socketConnection) {
        }

        void onReceiveFromPeer() override {
            char junk[4096];

            ssize_t ret = readFromPeer(junk, 4096);

            if (ret > 0) {
                std::size_t junklen = static_cast<std::size_t>(ret);
                VLOG(0) << "Data to reflect: " << std::string(junk, junklen);
                sendToPeer(junk, junklen);
            }
        }

        void onWriteError(int errnum) override {
            VLOG(0) << "OnWriteError: " << errnum;
        }

        void onReadError(int errnum) override {
            VLOG(0) << "OnReadError: " << errnum;
        }
    };

    class EchoSocketContextFactory : public core::socket::stream::SocketContextFactory {
    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            return new EchoSocketContext(socketConnection);
        }
    };

} // namespace apps::model

#endif // APPS_MODEL_ECHOSOCKETCONTEXT_H
