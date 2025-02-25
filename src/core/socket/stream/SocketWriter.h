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

#ifndef CORE_SOCKET_STREAM_SOCKETWRITER_H
#define CORE_SOCKET_STREAM_SOCKETWRITER_H

#include "core/eventreceiver/WriteEventReceiver.h"

namespace core::pipe {
    class Source;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <cstddef>
#include <functional>
#include <string>
#include <sys/types.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketWriter : public core::eventreceiver::WriteEventReceiver {
    public:
        SocketWriter() = delete;

    protected:
        explicit SocketWriter(const std::string& instanceName,
                              const std::function<void(int)>& onStatus,
                              const utils::Timeval& timeout,
                              std::size_t blockSize,
                              const utils::Timeval& terminateTimeout);

    private:
        void writeEvent() final;

        void signalEvent(int sigNum) final;

        void doWrite();

        virtual bool onSignal(int sigNum) = 0;
        virtual void doWriteShutdown(const std::function<void()>& onShutdown) = 0;

    protected:
        virtual ssize_t write(const char* chunk, std::size_t chunkLen);

        void setBlockSize(std::size_t writeBlockSize);

        void sendToPeer(const char* chunk, std::size_t chunkLen);
        bool streamToPeer(core::pipe::Source* source);
        void streamEof();

        void shutdownWrite(const std::function<void()>& onShutdown);

        bool markShutdown = false;

    private:
        std::function<void(int)> onStatus;

    protected:
        std::function<void()> onShutdown;

        std::vector<char> writePuffer;

        bool shutdownInProgress = false;

    private:
        core::pipe::Source* source = nullptr;

        std::size_t blockSize = 0;

    protected:
        utils::Timeval terminateTimeout;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETWRITER_H
