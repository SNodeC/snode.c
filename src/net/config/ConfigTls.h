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

#ifndef NET_CONFIG_CONFIGTLS_H
#define NET_CONFIG_CONFIGTLS_H

#include "net/config/ConfigBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include "utils/Timeval.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigTls : virtual public ConfigBase {
    public:
        ConfigTls();

        utils::Timeval getInitTimeout() const;
        utils::Timeval getShutdownTimeout() const;

        void setInitTimeout(const utils::Timeval& newInitTimeoutSet);
        void setShutdownTimeout(const utils::Timeval& newShutdownTimeoutSet);

    private:
        CLI::App* tlsSc = nullptr;
        CLI::Option* initTimeoutOpt = nullptr;
        CLI::Option* shutdownTimeoutOpt = nullptr;

        utils::Timeval initTimeout;
        utils::Timeval initTimeoutSet = -1;

        utils::Timeval shutdownTimeout;
        utils::Timeval shutdownTimeoutSet = -1;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGTLS_H
