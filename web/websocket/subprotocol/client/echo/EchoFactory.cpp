/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "EchoFactory.h"

#include "Echo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::subprotocol::echo::client {

    void EchoFactory::destroy() {
        delete this;
    }

    std::string EchoFactory::name() {
        return NAME;
    }

    web::websocket::client::SubProtocolFactory::SubProtocol* EchoFactory::create() {
        return new Echo();
    }

    extern "C" web::websocket::client::SubProtocolFactory* getSubProtocolFactory() {
        return new EchoFactory();
    }

} // namespace web::websocket::subprotocol::echo::client
