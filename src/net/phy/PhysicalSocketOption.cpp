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

#include "net/phy/PhysicalSocketOption.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::phy {

    PhysicalSocketOption::PhysicalSocketOption(int optLevel, int optName, int optValue)
        : optLevel(optLevel)
        , optName(optName)
        , optValue(reinterpret_cast<char*>(&optValue), reinterpret_cast<char*>(&optValue) + sizeof(optValue)) {
    }

    PhysicalSocketOption::PhysicalSocketOption(int optLevel, int optName, const std::string& optValue)
        : optLevel(optLevel)
        , optName(optName)
        , optValue(optValue.begin(), optValue.end()) {
    }

    PhysicalSocketOption::PhysicalSocketOption(int optLevel, int optName, const std::vector<char>& optValue)
        : optLevel(optLevel)
        , optName(optName)
        , optValue(optValue) {
    }

    int PhysicalSocketOption::getOptLevel() const {
        return optLevel;
    }

    int PhysicalSocketOption::getOptName() const {
        return optName;
    }

    const void* PhysicalSocketOption::getOptValue() const {
        return reinterpret_cast<const void*>(optValue.data());
    }

    socklen_t PhysicalSocketOption::getOptLen() const {
        return static_cast<socklen_t>(optValue.size());
    }

} // namespace net::phy
