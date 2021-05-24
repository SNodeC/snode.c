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

#ifndef WS_SERVER_WSSERVERPROTOCOL_H
#define WS_SERVER_WSSERVERPROTOCOL_H

namespace web::ws::server {
    class WSServerContextBase;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    class WSServerProtocol {
    public:
        WSServerProtocol();
        virtual ~WSServerProtocol();

        /* Facade (API) to WSServerContext -> WSTransmitter */
        void messageStart(char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void messageStart(const std::string& message, uint32_t messageKey = 0);

        void sendFrame(char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void sendFrame(const std::string& message, uint32_t messageKey = 0);

        void messageEnd(char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void messageEnd(const std::string& message, uint32_t messageKey = 0);

        void message(char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void message(const std::string& message, uint32_t messageKey = 0);

        static void broadcast(char* message, std::size_t messageLength, uint32_t messageKey = 0);
        static void broadcast(const std::string& message, uint32_t messageKey = 0);

        void sendPing(char* reason = nullptr, std::size_t reasonLength = 0);
        void close(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0);

        /* Callbacks (API) WSReceiver */
        virtual void onMessageStart(int opCode) = 0;
        virtual void onFrameData(const char* junk, std::size_t junkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onPongReceived() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        /* Callbacks (API) socketConnection -> WSServerContext */
        virtual void onProtocolConnect() = 0;
        virtual void onProtocolDisconnect() = 0;

        std::string getLocalAddressAsString() const;
        std::string getRemoteAddressAsString() const;

    private:
        void messageStart(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void message(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey = 0);
        static void broadcast(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey = 0);

    protected:
        WSServerContextBase* wSServerContext;

        template <typename WSServerProtocolT>
        friend class WSServerContext;

    private:
        static std::list<WSServerProtocol*> clients;
    };

} // namespace web::ws::server

#endif // WS_SERVER_WSSERVERPROTOCOL_H
