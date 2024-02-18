/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef WEB_HTTP_CLIENT_REQUESTCOMMANDS_H
#define WEB_HTTP_CLIENT_REQUESTCOMMANDS_H

namespace web::http::client {
    class RequestCommand;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    class RequestCommands {
    public:
        RequestCommands();
        RequestCommands(const RequestCommands&) = default;

        RequestCommands& operator=(const RequestCommands&) = default;

        ~RequestCommands();

    private:
        std::list<RequestCommand*> requestCommands;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_REQUESTCOMMANDS_H
