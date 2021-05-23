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

#include "web/ws/server/WSServerContextBase.h"
/*
namespace web::ws::server {

    template <typename WSServerProtocol>
    class WSServerContext;

}
*/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    class WSServerProtocol {
    public:
        virtual ~WSServerProtocol() = default;

        /* Facade to WSServerContext */
        void messageStart(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void sendFrame(char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void messageEnd(char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void message(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void sendPing(char* reason = nullptr, std::size_t reasonLength = 0);
        void close(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0);

        /* WSReceiver */
        virtual void onMessageStart(int opCode) = 0;
        virtual void onFrameData(const char* junk, std::size_t junkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onPongReceived() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

    protected:
        WSServerContextBase* wSServerContext;

        template <typename WSServerProtocolT>
        friend class WSServerContext;
    };

} // namespace web::ws::server

#endif // WS_SERVER_WSSERVERPROTOCOL_H
