/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/socket/LogicalSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    template <typename PhysicalSocket, typename Config>
    LogicalSocket<PhysicalSocket, Config>::LogicalSocket(const std::string& name)
        : config(std::make_shared<Config>(name)) {
    }

    template <typename PhysicalSocket, typename Config>
    LogicalSocket<PhysicalSocket, Config>::~LogicalSocket() {
    }

    template <typename PhysicalSocket, typename Config>
    Config& LogicalSocket<PhysicalSocket, Config>::getConfig() const {
        return *config;
    }

} // namespace core::socket
