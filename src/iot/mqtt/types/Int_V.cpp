/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "iot/mqtt/types/Int_V.h"

#include "iot/mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::types {

    Int_V::Int_V(SocketContext* socketContext)
        : mqtt::types::TypeBase(socketContext) {
    }

    Int_V::~Int_V() {
    }

    std::size_t Int_V::construct() {
        std::size_t consumed = 0;
        std::size_t ret = 0;

        do {
            char byte;
            ret = socketContext->readFromPeer(&byte, 1);

            if (ret > 0) {
                value += static_cast<uint64_t>((byte & 0x7F) * multiplier);
                if (multiplier > 0x80 * 0x80 * 0x80) {
                    error = true;
                } else {
                    multiplier *= 0x80;
                    completed = (byte & 0x80) == 0;
                }
                consumed += ret;
            }
        } while (ret > 0 && !completed && !error);

        return consumed;
    }

    std::uint64_t Int_V::getValue() {
        return value;
    }

    void Int_V::reset() {
        multiplier = 1;
        value = 0;

        mqtt::types::TypeBase::reset();
    }

} // namespace iot::mqtt::types
