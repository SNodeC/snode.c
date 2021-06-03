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

#ifndef WEB_WS_SERVER_WSUBSPROTOCOL_H
#define WEB_WS_SERVER_WSUBSPROTOCOL_H

#include "web/ws/WSSubProtocol.h"

namespace web::ws::server {
    class WSContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    class WSSubProtocol : public web::ws::WSSubProtocol {
    public:
        enum class Role { SERVER, CLIENT };

    protected:
        WSSubProtocol(const std::string& name);

        WSSubProtocol() = delete;
        WSSubProtocol(const WSSubProtocol&) = delete;
        WSSubProtocol& operator=(const WSSubProtocol&) = delete;

    public:
        virtual ~WSSubProtocol();

    public:
        /* Facade (API) to WSServerContext -> WSTransmitter to be used from WSSubProtocol-Subclasses */
        void sendBroadcast(const char* message, std::size_t messageLength);
        void sendBroadcast(const std::string& message);

        void sendBroadcastStart(const char* message, std::size_t messageLength);
        void sendBroadcastStart(const std::string& message);

        void sendBroadcastFrame(const char* message, std::size_t messageLength);
        void sendBroadcastFrame(const std::string& message);

        void sendBroadcastEnd(const char* message, std::size_t messageLength);
        void sendBroadcastEnd(const std::string& message);

    private:
        /* Callbacks (API) WSReceiver -> WSSubProtocol-Subclasses */
        virtual void onMessageStart(int opCode) = 0;
        virtual void onMessageData(const char* junk, std::size_t junkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onPongReceived() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        /* Callbacks (API) socketConnection -> WSSubProtocol-Subclasses */
        virtual void onProtocolConnected() = 0;
        virtual void onProtocolDisconnected() = 0;

        void sendBroadcast(uint8_t opCode, const char* message, std::size_t messageLength);
        void sendBroadcastStart(uint8_t opCode, const char* message, std::size_t messageLength);

        void setClients(std::list<WSSubProtocol*>* clients);

        std::list<WSSubProtocol*>* clients;

        friend class web::ws::server::WSContext;
    };

} // namespace web::ws::server

#endif // WEB_WS_SERVER_WSUBSPROTOCOL_H
