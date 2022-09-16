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

#include "mqtt/VarInteger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    std::string VarInteger::toVarString(uint32_t intValue) {
        std::string varString;

        uint32_t tmpValue = intValue;

        do {
            uint8_t encodedByte = tmpValue % 128;
            tmpValue = tmpValue / 128;

            if (tmpValue > 0) {
                encodedByte = encodedByte | 128;
            }

            varString.push_back(static_cast<char>(encodedByte));
        } while (tmpValue > 0);

        return varString;
    }

    uint32_t VarInteger::fromVarString([[maybe_unused]] const std::string& varStringIntValue) {
        /*
                int multiplier = 1;
                uint32_t value = 0;

                do {
                    uint32_t encodedByte = varStringIntValue;

                } while ((encodedByte & 128) != 0);
        */
        /*
            multiplier = 1

            value = 0

            do

                    encodedByte = 'next byte from stream'

                    value += (encodedByte AND 127) * multiplier

                             if (multiplier > 128*128*128)

                             throw Error(Malformed Variable Byte Integer)

                             multiplier *= 128

                    while ((encodedByte AND 128) != 0)

            }
        */

        return 0;
    }

} // namespace mqtt
