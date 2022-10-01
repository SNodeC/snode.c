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

#include "iot/mqtt1/types/UIntV.h"

#include "iot/mqtt1/types/TypeBase.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::types {

    std::size_t UIntV::construct(core::socket::SocketContext* socketContext) {
        std::size_t consumed = 0;
        std::size_t ret = 0;

        do {
            char byte;
            ret = socketContext->readFromPeer(&byte, 1);
            count += ret;
            error = count > sizeof(uint32_t);

            if (!error) {
                consumed += ret;
                complete = (byte & 0x80) == 0;

                value.push_back(byte & 0x7F);
            }
        } while (ret > 0 && !complete && !error);

        return consumed;
    }

    uint32_t UIntV::getValue() const {
        uint32_t uint32Value = 0;
        uint32_t multiplicator = 1;

        for (std::size_t i = 0; i < count; i++, multiplicator *= 0x80) {
            uint32Value += value[i] * multiplicator;
        }

        return uint32Value;
    }

    template class TypeBase<uint32_t>;

} // namespace iot::mqtt1::types
