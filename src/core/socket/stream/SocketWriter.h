/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstddef>
#include <functional>
#include <sys/types.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename PhysicalSocketT>
    class SocketWriter
        : public core::eventreceiver::WriteEventReceiver
        , virtual public PhysicalSocketT {
    public:
        SocketWriter() = delete;

    protected:
        using PhysicalSocket = PhysicalSocketT;

        explicit SocketWriter(const std::function<void(int)>& onError,
                              const utils::Timeval& timeout,
                              std::size_t blockSize,
                              const utils::Timeval& terminateTimeout);

        ~SocketWriter() override = default;

    private:
        virtual ssize_t write(const char* junk, std::size_t junkLen) = 0;

        void writeEvent() final;

        void doWrite();

    protected:
        void setBlockSize(std::size_t writeBlockSize);

        std::size_t sendToPeer(const char* junk, std::size_t junkLen);

        virtual void doWriteShutdown(const std::function<void(int)>& onShutdown);

        void shutdown(const std::function<void(int)>& onShutdown);

        void terminate() final;

    private:
        std::function<void(int)> onError;
        std::function<void(int)> onShutdown;

        std::vector<char> writeBuffer;
        std::size_t blockSize = 0;

        bool markShutdown = false;
        bool shutdownInProgress = false;
        bool terminateInProgress = false;

        utils::Timeval terminateTimeout;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETWRITER_H
