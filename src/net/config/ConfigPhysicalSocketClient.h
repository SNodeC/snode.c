/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef NET_CONFIG_CONFIGPHYSICALSOCKETCLIENT_H
#define NET_CONFIG_CONFIGPHYSICALSOCKETCLIENT_H

#include "net/config/ConfigPhysicalSocket.h" // IWYU pragma: export
#include "utils/Timeval.h"                   // IWYU pragma: export

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
}

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    class ConfigPhysicalSocketClient : public ConfigPhysicalSocket {
    private:
        using Super = ConfigPhysicalSocket;

    public:
        using Socket = ConfigPhysicalSocketClient;

    protected:
        explicit ConfigPhysicalSocketClient(ConfigInstance* instance);

    public:
        ConfigPhysicalSocketClient& setReconnect(bool reconnect = true);
        bool getReconnect() const;

        ConfigPhysicalSocketClient& setReconnectTime(double time);
        double getReconnectTime() const;

        ConfigPhysicalSocketClient& setConnectTimeout(const utils::Timeval& connectTimeout);
        utils::Timeval getConnectTimeout() const;

    private:
        CLI::Option* reconnectOpt = nullptr;
        CLI::Option* reconnectTimeOpt = nullptr;

        CLI::Option* connectTimeoutOpt = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGPHYSICALSOCKETCLIENT_H
