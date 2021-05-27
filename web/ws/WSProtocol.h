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

#ifndef WS_SERVER_WSPROTOCOL_H
#define WS_SERVER_WSPROTOCOL_H

namespace web::ws {
    class WSContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    class WSProtocol {
    protected:
        WSProtocol();
        virtual ~WSProtocol();

    public:
        /* Facade (API) to WSServerContext -> WSTransmitter to be used from WSProtocol-Subclasses */
        void sendMessageStart(const char* message, std::size_t messageLength);
        void sendMessageStart(const std::string& message);

        void sendMessageFrame(const char* message, std::size_t messageLength);
        void sendMessageFrame(const std::string& message);

        void sendMessageEnd(const char* message, std::size_t messageLength);
        void sendMessageEnd(const std::string& message);

        void sendMessage(const char* message, std::size_t messageLength);
        void sendMessage(const std::string& message);

        void sendPing(char* reason = nullptr, std::size_t reasonLength = 0);
        void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0);

        std::string getLocalAddressAsString() const;
        std::string getRemoteAddressAsString() const;

        static void sendBroadcast(const char* message, std::size_t messageLength);
        static void sendBroadcast(const std::string& message);

    private:
        /* Callbacks (API) WSReceiver -> WSProtocol-Subclasses */
        virtual void onMessageStart(int opCode) = 0;
        virtual void onMessageData(const char* junk, std::size_t junkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onPongReceived() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        /* Callbacks (API) socketConnection -> WSProtocol-Subclasses */
        virtual void onProtocolConnect() = 0;
        virtual void onProtocolDisconnect() = 0;

        void setWSContext(WSContext* wSServerContext);

        static void broadcast(uint8_t opCode, const char* message, std::size_t messageLength);

        static std::list<WSProtocol*> clients;

        WSContext* wSContext;

        friend class web::ws::WSContext;
    };

} // namespace web::ws

#endif // WS_SERVER_WSPROTOCOL_H
