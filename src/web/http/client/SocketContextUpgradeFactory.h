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

#ifndef WEB_HTTP_CLIENT_SOCKETCONTEXTUPGRADEFACTORY_H
#define WEB_HTTP_CLIENT_SOCKETCONTEXTUPGRADEFACTORY_H

#include "web/http/SocketContextUpgradeFactory.h" // IWYU pragma: export
// IWYU pragma: no_include "web/http/SocketContextUpgradeFactory.hpp"

namespace web::http::client {
    class Request;
    class Response;
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    class SocketContextUpgradeFactory
        : public web::http::SocketContextUpgradeFactory<web::http::client::Request, web::http::client::Response> {
    public:
        using Resquest = web::http::client::Request;
        using Reponse = web::http::client::Response;

    protected:
        SocketContextUpgradeFactory();

        using web::http::SocketContextUpgradeFactory<Request, Response>::prepare;
        virtual void prepare(Request& request) = 0;

    public:
        void checkRefCount() final;

        static void link(const std::string& upgradeContextName, SocketContextUpgradeFactory* (*linkedPlugin)());

        friend class SocketContextUpgradeFactorySelector;
    };

} // namespace web::http::client

extern template class web::http::SocketContextUpgradeFactory<web::http::client::Request, web::http::client::Response>;

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXTUPGRADEFACTORY_H
