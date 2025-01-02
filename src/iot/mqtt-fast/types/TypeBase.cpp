/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "iot/mqtt-fast/types/TypeBase.h"

#include "core/socket/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt_fast::types {

    TypeBase::TypeBase(core::socket::SocketContext* socketContext)
        : socketContext(socketContext) {
    }

    bool TypeBase::isComplete() const {
        return completed;
    }

    bool TypeBase::isError() const {
        return error;
    }

    std::size_t TypeBase::read(char* buf, std::size_t count) {
        std::size_t ret = 0;

        ret = socketContext->readFromPeer(buf, count);

        return ret;
    }

    void TypeBase::reset() {
        completed = false;
        error = false;
    }

} // namespace iot::mqtt_fast::types
