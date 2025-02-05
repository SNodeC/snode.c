/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef CORE_SOCKET_STREAM_SOCKETREADER_H
#define CORE_SOCKET_STREAM_SOCKETREADER_H

#include "core/eventreceiver/ReadEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <cstddef>
#include <functional>
#include <string>
#include <sys/types.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketReader : public core::eventreceiver::ReadEventReceiver {
    public:
        SocketReader() = delete;

    protected:
        explicit SocketReader(const std::string& instanceName,
                              const std::function<void(int)>& onStatus,
                              const utils::Timeval& timeout,
                              std::size_t blockSize,
                              const utils::Timeval& terminateTimeout);

    private:
        virtual void onReceivedFromPeer(std::size_t available) = 0;

        void readEvent() final;

        std::size_t doRead();
        void signalEvent(int sigNum) final;

    protected:
        virtual ssize_t read(char* chunk, std::size_t chunkLen);

        void setBlockSize(std::size_t readBlockSize);

        std::size_t readFromPeer(char* chunk, std::size_t chunkLen);

        void shutdownRead();

    private:
        std::function<void(int)> onStatus;

        std::vector<char> readBuffer;
        std::size_t blockSize = 0;

        std::size_t size = 0;
        std::size_t cursor = 0;

        bool shutdownInProgress = false;

    protected:
        utils::Timeval terminateTimeout;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETREADER_H
