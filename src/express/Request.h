/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "utils/AttributeInjector.h"
#include "web/http/server/Request.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class Request
        : public web::http::server::Request
        , public utils::MultibleAttributeInjector {
    public:
        Request() = default;
        explicit Request(web::http::server::Request&& request) noexcept;

        explicit Request(Request&) = delete;
        explicit Request(Request&&) noexcept = default;

        Request& operator=(Request&) = delete;
        Request& operator=(Request&&) noexcept = default;

        const std::string& param(const std::string& id, const std::string& fallBack = "");

        std::string originalUrl;
        std::string path;
        std::map<std::string, std::string> params;

    private:
        Request& extend();
    };

} // namespace express

#endif // EXPRESS_REQUEST_H
