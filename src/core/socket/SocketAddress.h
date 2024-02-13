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

#ifndef CORE_SOCKET_SOCKETADDRESS_H
#define CORE_SOCKET_SOCKETADDRESS_H

#include "core/socket/State.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <stdexcept>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket {

    class SocketAddress {
    public:
        class BadSocketAddress : public std::runtime_error {
        public:
            using Super = std::runtime_error;

            explicit BadSocketAddress(const State& state, const std::string& errorMessage, int errnum);
            ~BadSocketAddress() override;

            State getState() const;
            int getErrnum() const;

        private:
            State state;
            int errnum;
        };

        virtual ~SocketAddress();

        virtual bool useNext() {
            return false;
        }

        virtual std::string toString(bool expanded = true) const = 0;
    };

} // namespace core::socket

#endif // CORE_SOCKET_SOCKETADDRESS_H
