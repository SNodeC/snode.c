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

#ifndef NET_RF_STREAM_CONFIGBIND_H
#define NET_RF_STREAM_CONFIGBIND_H

#include "net/rf/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // for uint16_t
#include <string>  // for string

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rf::stream {

    class ConfigBind {
    public:
        explicit ConfigBind(CLI::App* baseSc);

        const std::string& getBindHost() const;

        uint8_t getBindChannel() const;

        SocketAddress getBindAddress() const;

    protected:
        void required(bool req) const;

        CLI::App* bindSc = nullptr;
        CLI::Option* bindHostOpt = nullptr;
        CLI::Option* bindChannelOpt = nullptr;

    private:
        std::string bindHost = "";
        uint8_t bindChannel = 0;
    };

} // namespace net::rf::stream

#endif