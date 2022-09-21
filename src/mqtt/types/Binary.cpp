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

#include "Binary.h"

#include "mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <iomanip>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::types {

    Binary::Binary(mqtt::SocketContext* socketContext)
        : mqtt::types::TypesBase(socketContext)
        , length(socketContext) {
    }

    Binary::~Binary() {
    }

    std::size_t Binary::construct() {
        std::size_t consumed = 0;

        switch (state) {
            case 0:
                consumed = length.construct();
                if (length.isCompleted()) {
                    state++;
                    needed = stillNeeded = length.getValue();
                    data.resize(stillNeeded, '\0');
                    //                    VLOG(0) << "Remaining Length: " << length.getValue();
                } else if (length.isError()) {
                    error = 0;
                }
                break;
            case 1:
                consumed = socketContext->readFromPeer(data.data() + needed - stillNeeded, stillNeeded);
                stillNeeded -= consumed;
                if (stillNeeded == 0) {
                    //                    VLOG(0) << "Binary completed: " << data.data();
                    //                    int b = 1;
                    //                    for (char val : data) {
                    //                        VLOG(0) << std::setfill(' ') << std::setw(3) << b++ << ". " << std::setfill(' ') <<
                    //                        std::setw(3) << val << " - "
                    //                                << "0x" << std::hex << std::setfill('0') << std::setw(2) <<
                    //                                static_cast<uint16_t>(val) << std::dec;
                    //                    }
                    completed = true;
                }
                break;
        }

        return consumed;
    }

    std::vector<char>& Binary::getValue() {
        return data;
    }

    void Binary::reset() {
        state = 0;
        needed = 0;
        stillNeeded = 0;
        length.reset();
        data.clear();

        mqtt::types::TypesBase::reset();
    }

} // namespace mqtt::types
