/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef CORE_SOCKET_STREAM_SOCKETCONTEXT_H
#define CORE_SOCKET_STREAM_SOCKETCONTEXT_H

#include "core/socket/SocketContext.h"

namespace core::pipe {
    class Source;
}

namespace core::socket::stream {
    class SocketConnection;

} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <chrono>
#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketContext : public core::socket::SocketContext {
    private:
        using Super = core::socket::SocketContext;

    public:
        explicit SocketContext(core::socket::stream::SocketConnection* socketConnection);

        ~SocketContext() override;

        using Super::readFromPeer;
        using Super::sendToPeer;

        void sendToPeer(const char* chunk, std::size_t chunkLen) const final;
        bool streamToPeer(core::pipe::Source* source) const;
        void streamEof();

        std::size_t readFromPeer(char* chunk, std::size_t chunklen) const final;

        void setTimeout(const utils::Timeval& timeout) final;

        void shutdownRead();
        void shutdownWrite(bool forceClose = false);
        void close() override;

        std::size_t getTotalSent() const override;
        std::size_t getTotalQueued() const override;

        std::size_t getTotalRead() const override;
        std::size_t getTotalProcessed() const override;

        std::string getOnlineSince() const override;
        std::string getOnlineDuration() const override;

        SocketConnection* getSocketConnection() const;
        virtual void switchSocketContext(SocketContext* newSocketContext);

    protected:
        void onWriteError(int errnum) override;
        void onReadError(int errnum) override;

        void readFromPeer(std::size_t available);

    private:
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;

        virtual void attach();
        virtual void detach();

        static std::string timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint);
        static std::string
        durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
                         const std::chrono::time_point<std::chrono::system_clock>& later = std::chrono::system_clock::now());

        core::socket::stream::SocketConnection* socketConnection;

        core::socket::stream::SocketContext* newSocketContext = nullptr;

        std::string onlineSince;
        std::size_t alreadyTotalQueued = 0;
        std::size_t alreadyTotalProcessed = 0;

        std::chrono::time_point<std::chrono::system_clock> onlineSinceTimePoint;

        template <typename PhysicalSocketT, class SocketReaderT, class SocketWriterT, typename ConfigT>
        friend class SocketConnectionT;
        friend class SocketConnection;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONTEXT_H
