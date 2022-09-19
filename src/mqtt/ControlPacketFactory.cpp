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

#include "mqtt/ControlPacketFactory.h"

#include "mqtt/SocketContext.h"
#include "mqtt/types/Int_V.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <iomanip>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    ControlPacketFactory::ControlPacketFactory(mqtt::SocketContext* socketContext)
        : socketContext(socketContext)
        , type(socketContext)
        , data(socketContext) {
        state = 1;
    }

    std::size_t ControlPacketFactory::construct() {
        std::size_t consumed = 0;

        switch (state) {
            case 1:
                consumed = type.construct();
                if (type.isCompleted()) {
                    state++;
                }
                break;
            case 2:
                consumed = data.construct();
                if (data.isCompleted()) {
                    state++;
                    VLOG(0) << "Completed -------------------";
                }
                break;
        }

        return consumed;
    }

    bool ControlPacketFactory::complete() {
        return completed;
    }

    bool ControlPacketFactory::isError() {
        return error;
    }

    ControlPacket* ControlPacketFactory::get() {
        return nullptr;
    }

} // namespace mqtt
