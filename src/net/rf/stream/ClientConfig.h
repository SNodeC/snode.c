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

#ifndef NET_RF_STREAM_CLIENTCONFIG_H
#define NET_RF_STREAM_CLIENTCONFIG_H

#include "net/ConfigBacklog.h"
#include "net/ConfigBase.h"
#include "net/ConfigConn.h"
#include "net/rf/stream/ConfigBind.h"
#include "net/rf/stream/ConfigConnect.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rf::stream {

    class ClientConfig
        : public net::ConfigBase
        , public net::ConfigBacklog
        , public net::rf::stream::ConfigConnect
        , public net::rf::stream::ConfigBind
        , public net::ConfigConn {
    public:
        explicit ClientConfig(const std::string& name);

    private:
        void required(bool req) const final;
    };

} // namespace net::rf::stream

#endif // NET_RF_STREAM_CLIENTCONFIG_H
