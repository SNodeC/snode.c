/*
 * snode.c - a slim toolkit for network communication
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

#include "express/middleware/VerboseRequest.h"

#include "core/socket/stream/SocketConnection.h"
#include "web/http/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "web/http/http_utils.h"

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace express::middleware {

    VerboseRequest::VerboseRequest(Details details) {
        use([details] MIDDLEWARE(req, res, next) {
            LOG(DEBUG) << res->getSocketContext()->getSocketConnection()->getConnectionName()
                       << " Express VerboseMiddleware: " << req->method << " " << req->url << " " << req->httpVersion << "\n"
                       << httputils::toString(
                              req->method,
                              req->url,
                              req->httpVersion,
                              (details & Details::W_QUERIES) == Details::W_QUERIES ? req->queries : web::http::CiStringMap<std::string>(),
                              (details & Details::W_HEADERS) == Details::W_HEADERS ? req->headers : web::http::CiStringMap<std::string>(),
                              (details & Details::W_COOKIES) == Details::W_COOKIES ? req->cookies : web::http::CiStringMap<std::string>(),
                              (details & Details::W_CONTENT) == Details::W_CONTENT ? req->body : std::vector<char>());

            next();
        });
    }

    class VerboseRequest& VerboseRequest(VerboseRequest::Details details) {
        static std::shared_ptr<class VerboseRequest> verboseRequest = nullptr;

        if (verboseRequest == nullptr) {
            verboseRequest = std::shared_ptr<class VerboseRequest>(new class VerboseRequest(details));
        }

        return *verboseRequest;
    }

} // namespace express::middleware
