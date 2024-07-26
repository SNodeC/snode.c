/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef NET_CONFIG_CONFIGADDRESSREVERSE_H
#define NET_CONFIG_CONFIGADDRESSREVERSE_H

#include "net/config/ConfigAddressBase.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    template <typename SocketAddressT>
    class ConfigAddressReverse : public net::config::ConfigAddressBase<SocketAddressT> {
    private:
        using Super = net::config::ConfigAddressBase<SocketAddressT>;

    protected:
        using Super::Super;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGADDRESSREVERSE_H
