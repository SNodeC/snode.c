/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef EXPRESS_RESPONSE_H
#define EXPRESS_RESPONSE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "server/Response.h"

namespace express {

    class Response : public http::server::Response {
    public:
        Response(http::server::ServerContextBase* serverContext);

        void sendFile(const std::string& file, const std::function<void(int err)>& onError);
        void download(const std::string& file, const std::function<void(int err)>& onError);
        void download(const std::string& file, const std::string& name, const std::function<void(int err)>& onError);

        void redirect(const std::string& name);
        void redirect(int status, const std::string& name);

        void sendStatus(int status);

        void reset() override;
    };

} // namespace express

#endif // EXPRESS_RESPONSE_H
