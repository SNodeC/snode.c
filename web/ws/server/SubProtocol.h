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

#ifndef WEB_WS_SERVER_SUBSPROTOCOL_H
#define WEB_WS_SERVER_SUBSPROTOCOL_H

#include "web/ws/SubProtocol.h"

namespace web::ws::server {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    class SubProtocol : public web::ws::SubProtocol {
    public:
        enum class Role { SERVER, CLIENT };

    protected:
        SubProtocol(const std::string& name);

        SubProtocol() = delete;
        SubProtocol(const SubProtocol&) = delete;
        SubProtocol& operator=(const SubProtocol&) = delete;

    public:
        virtual ~SubProtocol();

    public:
        /* Facade (API) to WSServerContext -> WSTransmitter to be used from SubProtocol-Subclasses */
        void sendBroadcast(const char* message, std::size_t messageLength);
        void sendBroadcast(const std::string& message);

        void sendBroadcastStart(const char* message, std::size_t messageLength);
        void sendBroadcastStart(const std::string& message);

        void sendBroadcastFrame(const char* message, std::size_t messageLength);
        void sendBroadcastFrame(const std::string& message);

        void sendBroadcastEnd(const char* message, std::size_t messageLength);
        void sendBroadcastEnd(const std::string& message);

        void forEachClient(const std::function<void(SubProtocol*)>& forEachClient);

    private:
        /* Callbacks (API) WSReceiver -> SubProtocol-Subclasses */
        void onMessageStart(int opCode) override = 0;
        void onMessageData(const char* junk, std::size_t junkLen) override = 0;
        void onMessageEnd() override = 0;
        void onPongReceived() override = 0;
        void onMessageError(uint16_t errnum) override = 0;

        /* Callbacks (API) socketConnection -> SubProtocol-Subclasses */
        void onProtocolConnected() override = 0;
        void onProtocolDisconnected() override = 0;

        void sendBroadcast(uint8_t opCode, const char* message, std::size_t messageLength);
        void sendBroadcastStart(uint8_t opCode, const char* message, std::size_t messageLength);

        void setClients(std::list<SubProtocol*>* clients);

        std::list<SubProtocol*>* clients;

        friend class web::ws::server::SocketContext;
    };

} // namespace web::ws::server

#endif // WEB_WS_SERVER_SUBSPROTOCOL_H
