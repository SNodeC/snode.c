/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef WEB_WEBSOCKET_SUBSPROTOCOL_H
#define WEB_WEBSOCKET_SUBSPROTOCOL_H

#include "core/timer/Timer.h" // IWYU pragma: export

namespace core::socket::stream {
    class SocketConnection;
}

namespace web::websocket {
    template <typename SubProtocolT, typename RequestT, typename ResponseT>
    class SocketContextUpgrade;
    class SubProtocolContext;
} // namespace web::websocket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SocketContextUpgradeT>
    class SubProtocol {
    public:
        SubProtocol(SubProtocol&) = delete;
        SubProtocol(SubProtocol&&) = delete;

        SubProtocol& operator=(SubProtocol&) = delete;
        SubProtocol& operator=(SubProtocol&&) = delete;

    private:
        using SocketContextUpgrade = SocketContextUpgradeT;

    protected:
        SubProtocol(SubProtocolContext* subProtocolContext, const std::string& name, int pingInterval = 60, int maxFlyingPings = 3);
        virtual ~SubProtocol();

    public:
        /* Facade (API) to WSServerContext -> WSTransmitter to be used from SubProtocol-Subclasses */
        void sendMessage(const char* message, std::size_t messageLength) const;
        void sendMessage(const std::string& message) const;
        void sendMessageStart(const char* message, std::size_t messageLength) const;
        void sendMessageStart(const std::string& message) const;
        void sendMessageFrame(const char* message, std::size_t messageLength) const;
        void sendMessageFrame(const std::string& message) const;
        void sendMessageEnd(const char* message, std::size_t messageLength) const;
        void sendMessageEnd(const std::string& message) const;
        void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0) const;
        void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0);

    private:
        /* Callbacks (API) socketConnection -> SubProtocol-Subclasses */
        void attach();
        void detach();
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;
        virtual bool onSignal(int sig) = 0;

        /* Callbacks (API) WSReceiver -> SubProtocol-Subclasses */
        virtual void onMessageStart(int opCode) = 0;
        virtual void onMessageData(const char* chunk, std::size_t chunkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        virtual void onPongReceived();

    public:
        const std::string& getName();
        core::socket::stream::SocketConnection* getSocketConnection() const;

        std::size_t getPayloadTotalSent() const;
        std::size_t getPayloadTotalRead() const;

        std::string getOnlineSince() const;
        std::string getOnlineDuration() const;

    private:
        const std::string name;

    protected:
        SubProtocolContext* subProtocolContext;

    private:
        core::timer::Timer pingTimer;
        int flyingPings = 0;

        template <typename SubProtocolT, typename RequestT, typename ResponseT>
        friend class web::websocket::SocketContextUpgrade;
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SUBSPROTOCOL_H
