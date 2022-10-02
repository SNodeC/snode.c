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

#include "iot/mqtt1/types/String.h"

#include "iot/mqtt1/types/TypeBase.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt1::types {

    std::size_t String::construct(core::socket::SocketContext* socketContext) {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = stringLength.construct(socketContext);

                if (stringLength.isComplete()) {
                    setSize(stringLength.getValue());
                    state++;
                } else {
                    break;
                }
                [[fallthrough]];
            case 1:
                consumed += TypeBase::construct(socketContext);
                break;
            default:
                break;
        }

        return consumed;
    }

    void String::setValue(const std::string& newValue) {
        value = std::vector<char>(newValue.begin(), newValue.end());
    }

    std::string String::getValue() const {
        return std::string(value.begin(), value.end());
    }

    template class TypeBase<std::string>;

} // namespace iot::mqtt1::types
