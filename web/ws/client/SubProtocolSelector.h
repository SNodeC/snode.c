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

#ifndef WEB_WS_CLIENT_SUBPROTOCOLSELECTOR_H
#define WEB_WS_CLIENT_SUBPROTOCOLSELECTOR_H

#include "web/ws/SubProtocolSelector.h"
#include "web/ws/client/SubProtocolInterface.h"

namespace web::ws {
    class SubProtocol;
} // namespace web::ws

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::client {

    class SubProtocolSelector : public web::ws::SubProtocolSelector {
    public:
        SubProtocolSelector();

    private:
        using web::ws::SubProtocolSelector::loadSubProtocols;

    public:
        void loadSubProtocols() override;

        web::ws::SubProtocol* select(const std::string& subProtocolName) override;

        static std::shared_ptr<SubProtocolSelector> instance();

    private:
        static std::shared_ptr<SubProtocolSelector> subProtocolSelector;
    };

} // namespace web::ws::client

#endif // WEB_WS_CLIENT_SUBPROTOCOLSELECTOR_H
