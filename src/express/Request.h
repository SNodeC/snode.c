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

namespace web::http::server {
    class Request;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class Request : public utils::MultibleAttributeInjector {
    public:
        Request() = default;
        explicit Request(const std::shared_ptr<web::http::server::Request>& request) noexcept;

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

        std::shared_ptr<web::http::server::Request> requestBase;

        friend class Response;

        // Facade to web::http::server::Request (requestBase)
    public:
        const std::string& get(const std::string& key, int i = 0) const;
        const std::string& cookie(const std::string& key) const;
        const std::string& query(const std::string& key) const;

        // Properties
        std::string method;
        std::string url;
        std::string httpVersion;
        int httpMajor = 0;
        int httpMinor = 0;

        std::map<std::string, std::string> queries;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> cookies;
        std::vector<uint8_t> body;
    };

} // namespace express

#endif // EXPRESS_REQUEST_H
