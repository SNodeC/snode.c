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

#ifndef NET_STREAM_CONFIG_LEGACY_CONFIGSOCKETSERVER_H
#define NET_STREAM_CONFIG_LEGACY_CONFIGSOCKETSERVER_H

#include "net/config/ConfigInstance.h" // IWYU pragma: export
#include "net/config/ConfigLegacy.h"   // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config::stream::legacy {

    template <typename ConfigSocketServerBaseT>
    class ConfigSocketServer
        : public net::config::ConfigInstance
        , public ConfigSocketServerBaseT
        , public net::config::ConfigLegacy {
    protected:
        explicit ConfigSocketServer(const std::string& name);
    };

} // namespace net::config::stream::legacy

#endif // NET_STREAM_CONFIG_LEGACY_CONFIGSOCKETSERVER_H
