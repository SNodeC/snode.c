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

#ifndef WSTRANSCEIVER_H
#define WSTRANSCEIVER_H

#include "http/server/ServerContext.h"
#include "http/websocket/WSReceiver.h"
#include "http/websocket/WSTransmitter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    class WSServerContext
        : public http::server::ServerContext
        , public WSReceiver
        , public WSTransmitter {
    private:
        void receiveData(const char* junk, std::size_t junklen) override;
        void onReadError(int errnum) override;
        void onWriteError(int errnum) override;

        void onMessageStart(int opCode) override;
        void onMessageData(char* junk, uint64_t junkLen) override;
        void onMessageEnd() override;

        void onFrameReady(char* frame, uint64_t frameLength) override;
    };

} // namespace http::websocket

#endif // WSTRANSCEIVER_H
