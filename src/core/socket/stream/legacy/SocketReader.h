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

#ifndef CORE_SOCKET_STREAM_LEGACY_SOCKETREADER_H
#define CORE_SOCKET_STREAM_LEGACY_SOCKETREADER_H

#include "core/socket/stream/SocketReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::legacy {

    template <typename SocketT>
    class SocketReader : public core::socket::stream::SocketReader<SocketT> {
    private:
        using Super = core::socket::stream::SocketReader<SocketT>;
        using Super::Super;

        ssize_t read(char* junk, std::size_t junkLen) override {
            return core::system::recv(this->getFd(), junk, junkLen, 0);
        }

        bool continueReadImmediately() const override {
            return Super::continueReadImmediately();
        }

    protected:
        void terminate() override {
            Super::terminate();
        }
    };

} // namespace core::socket::stream::legacy

#endif // CORE_SOCKET_STREAM_LEGACY_SOCKETREADER_H
