/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
        virtual ssize_t write(const char* junk, std::size_t junkLen) = 0;

        void writeEvent() final;

        void signalEvent(int sigNum) final;

        void doWrite();

        [[nodiscard]] virtual bool onSignal(int sigNum) = 0;
        virtual void doWriteShutdown(const std::function<void()>& onShutdown) = 0;

    protected:
        void setBlockSize(std::size_t writeBlockSize);

        void sendToPeer(const char* junk, std::size_t junkLen);
        [[nodiscard]] bool streamToPeer(core::pipe::Source* source);

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

        utils::Timeval terminateTimeout;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETWRITER_H
