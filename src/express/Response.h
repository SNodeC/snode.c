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

#ifndef EXPRESS_RESPONSE_H
#define EXPRESS_RESPONSE_H

#include "web/http/server/Response.h" // IWYU pragma: export

namespace web::http::server {
    class RequestContextBase;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json_fwd.hpp>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class Response : public web::http::server::Response {
    public:
        explicit Response(web::http::server::RequestContextBase* requestContext);

        ~Response() override;

        void json(const nlohmann::json& json);

        void download(const std::string& file, const std::function<void(int err)>& onError);
        void download(const std::string& file, const std::string& fileName, const std::function<void(int err)>& onError);

        void redirect(const std::string& loc);
        void redirect(int state, const std::string& loc);

        void sendStatus(int state);

        Response& attachment(const std::string& fileName = "");
        Response& location(const std::string& loc);
        Response& vary(const std::string& field);
    };

} // namespace express

#endif // EXPRESS_RESPONSE_H
