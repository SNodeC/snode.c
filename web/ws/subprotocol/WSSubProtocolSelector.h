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

#ifndef WEB_WS_SUBPROTOCOL_CHOOSER_H
#define WEB_WS_SUBPROTOCOL_CHOOSER_H

#include "WSSubProtocolPluginInterface.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::subprotocol {

    class WSSubProtocolSelector {
    public:
        WSSubProtocolSelector();
        ~WSSubProtocolSelector();

        void loadSubProtocols(const std::string& path);
        void loadSubProtocol(const WSSubProtocolPluginInterface& wSProtocolPlugin);

        WSSubProtocolPluginInterface* select(const std::string& subProtocol, web::ws::WSSubProtocol::Role role);

    protected:
        void loadSubProtocols();

    private:
        std::map<std::string, WSSubProtocolPluginInterface> serverSubprotocols;
        std::map<std::string, WSSubProtocolPluginInterface> clientSubprotocols;
    };

} // namespace web::ws::subprotocol

#endif // WEB_WS_SUBPROTOCOL_CHOOSER_H
