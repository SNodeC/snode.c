/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AttributeInjector.h"

class SocketConnectionBase {
public:
    SocketConnectionBase() = default;
    SocketConnectionBase(const SocketConnectionBase&) = delete;
    SocketConnectionBase& operator=(const SocketConnectionBase&) = delete;

    virtual ~SocketConnectionBase() = default;

    virtual void enqueue(const char* buffer, int size) = 0;
    virtual void enqueue(const std::string& data) = 0;

    virtual void end(bool instantly = false) = 0;

    template <utils::InjectedAttribute Attribute>
    constexpr void setProtocol(Attribute& attribute) const {
        protocol.setAttribute<Attribute>(attribute);
    }

    template <utils::InjectedAttribute Attribute>
    constexpr void setProtocol(Attribute&& attribute) const {
        protocol.setAttribute<Attribute>(attribute);
    }

    template <utils::InjectedAttribute Attribute>
    constexpr bool getProtocol(std::function<void(Attribute&)> onFound) const {
        return protocol.getAttribute<Attribute>([&onFound](Attribute& attribute) -> void {
            onFound(attribute);
        });
    }

    template <utils::InjectedAttribute Attribute>
    constexpr void getProtocol(std::function<void(Attribute&)> onFound, std::function<void(const std::string&)> onNotFound) const {
        return protocol.getAttribute<Attribute>(
            [&onFound](Attribute& attribute) -> void {
                onFound(attribute);
            },
            [&onNotFound](const std::string& msg) -> void {
                onNotFound(msg);
            });
    }

private:
    utils::SingleAttributeInjector protocol;
};

#endif // SOCKETCONNECTION_H
