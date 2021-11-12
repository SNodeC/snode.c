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

#include "web/http/client/Response.h"

#include "log/Logger.h"
#include "web/http/CookieOptions.h"
#include "web/http/SocketContext.h"
#include "web/http/client/SocketContextUpgradeFactorySelector.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iterator> // for advance, distance
#include <utility>  // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    Response::Response(SocketContext* clientContext)
        : socketContext(clientContext) {
    }

    const std::string& Response::header(const std::string& key, int i) const {
        std::string tmpKey = key;
        httputils::to_lower(tmpKey);

        if (headers->find(tmpKey) != headers->end()) {
            std::pair<std::multimap<std::string, std::string>::const_iterator, std::multimap<std::string, std::string>::const_iterator>
                range = headers->equal_range(tmpKey);

            if (std::distance(range.first, range.second) >= i) {
                std::advance(range.first, i);
                return (*(range.first)).second;
            } else {
                return nullstr;
            }
        } else {
            return nullstr;
        }
    }

    const std::string& Response::cookie(const std::string& key) const {
        std::map<std::string, CookieOptions>::const_iterator it;

        std::string tmpKey = key;
        httputils::to_lower(tmpKey);

        if ((it = cookies->find(tmpKey)) != cookies->end()) {
            return it->second.getValue();
        } else {
            return nullstr;
        }
    }

    std::size_t Response::bodyLength() const {
        return contentLength;
    }

    void Response::upgrade(Request& request) {
        if (httputils::ci_contains(this->header("connection"), "Upgrade")) {
            web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory =
                web::http::client::SocketContextUpgradeFactorySelector::instance()->select(request, *this);

            if (socketContextUpgradeFactory != nullptr) {
                if (socketContext->switchSocketContext(socketContextUpgradeFactory) == nullptr) {
                    socketContext->close();
                }
            } else {
                VLOG(0) << "SocketContextUpgradeFactory not existing";
                socketContext->close();
            }
        } else {
            VLOG(0) << "Response did not contain upgrade";
            socketContext->close();
        }
    }

    void Response::reset() {
    }

} // namespace web::http::client
