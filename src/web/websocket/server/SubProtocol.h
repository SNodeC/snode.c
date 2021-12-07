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

#ifndef WEB_WEBSOCKET_SERVER_SUBSPROTOCOL_H
#define WEB_WEBSOCKET_SERVER_SUBSPROTOCOL_H

#include "web/websocket/SubProtocol.h"                 // IWYU pragma: export
#include "web/websocket/server/SocketContextUpgrade.h" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    class SubProtocol : public web::websocket::SubProtocol<web::websocket::server::SocketContextUpgrade> {
    private:
        using Super = web::websocket::SubProtocol<web::websocket::server::SocketContextUpgrade>;

    protected:
        SubProtocol(const std::string& name);

    public:
        ~SubProtocol();

        /* Facade (API) to WSServerContext -> WSTransmitter to be used from SubProtocol-Subclasses */
        using Super::sendMessage;

        using Super::sendMessageEnd;
        using Super::sendMessageFrame;
        using Super::sendMessageStart;

        using Super::sendClose;
        using Super::sendPing;

        void subscribe(const std::string& channel);

        void sendBroadcast(const char* message, std::size_t messageLength, bool excludeSelf = false);
        void sendBroadcast(const std::string& message, bool excludeSelf = false);

        void sendBroadcastStart(const char* message, std::size_t messageLength, bool excludeSelf = false);
        void sendBroadcastStart(const std::string& message, bool excludeSelf = false);

        void sendBroadcastFrame(const char* message, std::size_t messageLength, bool excludeSelf = false);
        void sendBroadcastFrame(const std::string& message, bool excludeSelf = false);

        void sendBroadcastEnd(const char* message, std::size_t messageLength, bool excludeSelf = false);
        void sendBroadcastEnd(const std::string& message, bool excludeSelf = false);

        void forEachClient(const std::function<void(SubProtocol*)>& sendToClient, bool excludeSelf = false);

    private:
        /* Callbacks (API) WSReceiver -> SubProtocol-Subclasses */
        void onMessageStart(int opCode) override = 0;
        void onMessageData(const char* junk, std::size_t junkLen) override = 0;
        void onMessageEnd() override = 0;
        void onPongReceived() override = 0;
        void onMessageError(uint16_t errnum) override = 0;

        /* Callbacks (API) socketConnection -> SubProtocol-Subclasses */
        void onConnected() override = 0;
        void onDisconnected() override = 0;

        std::string group;

        template <typename RequestT, typename ResponseT, typename SubProtocolT>
        friend class web::websocket::SocketContextUpgrade;

        friend class GroupsManager;
    };

} // namespace web::websocket::server

#endif // WEB_WEBSOCKET_SERVER_SUBSPROTOCOL_H
