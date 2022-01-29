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

#ifndef NET_IN_STREAM_SERVERCONFIG_H
#define NET_IN_STREAM_SERVERCONFIG_H

#include "net/ServerConfig.h"
#include "net/in/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream {

    class ServerConfig : public net::ServerConfig {
    public:
        explicit ServerConfig(const std::string& name);

        const std::string& getBindInterface() const;

        uint16_t getBindPort() const;

        net::in::SocketAddress getSocketAddress() const;

        int parse(bool required = false) const;

    private:
        CLI::App* serverBindSc = nullptr;
        CLI::Option* bindServerHostOpt = nullptr;
        CLI::Option* bindServerPortOpt = nullptr;

    protected:
        std::string bindInterface;
        uint16_t bindPort;
    };

} // namespace net::in::stream

#endif
