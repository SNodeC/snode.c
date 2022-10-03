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

#include "iot/mqtt1/types/TypeBase.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::types {

    UIntV::UIntV()
        : TypeBase(0) {
    }

    std::size_t UIntV::construct(core::socket::SocketContext* socketContext) {
        std::size_t consumed = 0;
        std::size_t ret = 0;

        do {
            char byte;
            ret = socketContext->readFromPeer(&byte, 1);
            consumed += ret;

            value.push_back(byte);
            this->length++;

            if (value.size() > sizeof(uint32_t)) {
                error = true;
            } else {
                complete = (byte & 0x80) == 0;
            }
        } while (ret > 0 && !complete && !error);

        return consumed;
    }

    void UIntV::setValue(const uint32_t& newValue) {
        uint32_t remainingValue = newValue;
        value.resize(0);

        do {
            uint8_t encodedByte = static_cast<uint8_t>(remainingValue % 0x80);

            remainingValue /= 0x80;
            if (remainingValue > 0) {
                encodedByte |= 0x80;
            }

            value.push_back(static_cast<char>(encodedByte));
        } while (remainingValue > 0);
    }

    uint32_t UIntV::getValue() const {
        uint32_t uint32Value = 0;
        uint32_t multiplicator = 1;

        for (std::size_t i = 0; i < value.size(); i++, multiplicator *= 0x80) {
            uint32Value += static_cast<uint8_t>(value[i] & 0xFF) * multiplicator;
        }

        return uint32Value;
    }

    void UIntV::reset([[maybe_unused]] std::size_t size) {
        TypeBase::reset(0);
    }

    //    template class TypeBase<uint32_t>;

} // namespace iot::mqtt1::types
