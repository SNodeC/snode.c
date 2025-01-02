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

#ifndef NET_PHY_PHYSICALSOCKETOPTIONS_H
#define NET_PHY_PHYSICALSOCKETOPTIONS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <sys/socket.h>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::phy {

    class PhysicalSocketOption {
    public:
        PhysicalSocketOption() = default;

        PhysicalSocketOption(int optLevel, int optName, int optValue);
        PhysicalSocketOption(int optLevel, int optName, const std::string& optValue);
        PhysicalSocketOption(int optLevel, int optName, const std::vector<char>& optValue);

        int getOptLevel() const;
        int getOptName() const;
        const void* getOptValue() const;
        socklen_t getOptLen() const;

    private:
        int optLevel = -1;
        int optName = -1;
        std::vector<char> optValue;
    };

} // namespace net::phy

#endif // NET_PHY_PHYSICALSOCKETOPTIONS_H
