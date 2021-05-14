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

#ifndef NET_SOCKET_STREAM_SOCKETCONNECTIONBASE_H
#define NET_SOCKET_STREAM_SOCKETCONNECTIONBASE_H

#include "utils/AttributeInjector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    class SocketConnectionBase {
    protected:
        SocketConnectionBase() = default;
        SocketConnectionBase(const SocketConnectionBase&) = delete;
        SocketConnectionBase& operator=(const SocketConnectionBase&) = delete;

        virtual ~SocketConnectionBase() = default;

    public:
        virtual void enqueue(const char* junk, std::size_t junkLen) = 0;
        virtual void enqueue(const std::string& data) = 0;

        virtual void close(bool instantly = false) = 0;

        template <utils::InjectableAttribute Attribute>
        constexpr void setContext(Attribute& attribute) const {
            protocol.setAttribute<Attribute>(attribute, true);
        }

        template <utils::InjectableAttribute Attribute>
        constexpr void setContext(Attribute&& attribute) const {
            protocol.setAttribute<Attribute>(attribute, true);
        }

        template <utils::InjectableAttribute Attribute>
        constexpr bool getContext(const std::function<void(Attribute&)>& onFound) const {
            return protocol.getAttribute<Attribute>(onFound);
        }

        template <utils::InjectableAttribute Attribute>
        constexpr void getContext(const std::function<void(Attribute&)>& onFound,
                                  const std::function<void(const std::string&)>& onNotFound) const {
            return protocol.getAttribute<Attribute>(onFound, onNotFound);
        }

    private:
        utils::SingleAttributeInjector protocol;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETCONNECTIONBASE_H
