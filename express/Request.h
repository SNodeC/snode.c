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

#ifndef EXPRESS_REQUEST_H
#define EXPRESS_REQUEST_H

#include "web/http/server//Request.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class Request : public web::http::server::Request {
    public:
        std::string& param(const std::string& id);

        std::string originalUrl;
        std::string path;
        std::map<std::string, std::string> params;

    protected:
        void extend();

        template <typename ServerT>
        friend class WebAppT;
    };

} // namespace express

#endif // EXPRESS_REQUEST_H
