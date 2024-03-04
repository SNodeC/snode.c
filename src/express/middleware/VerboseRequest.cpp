/*
 * snode.c - a slim toolkit for network communication
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

#include "express/middleware/VerboseRequest.h"

#include "core/socket/stream/SocketConnection.h"
#include "web/http/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/hexdump.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace express::middleware {

    VerboseRequest::VerboseRequest(Details details) {
        use([details] MIDDLEWARE(req, res, next) {
            const int prefixLength = 9;
            int keyLength = 0;

            for (const auto& [key, value] : req->queries) {
                keyLength = std::max(keyLength, static_cast<int>(key.size()));
            }
            for (const auto& [key, value] : req->headers) {
                keyLength = std::max(keyLength, static_cast<int>(key.size()));
            }
            for (const auto& [key, value] : req->cookies) {
                keyLength = std::max(keyLength, static_cast<int>(key.size()));
            }

            std::stringstream requestStream;

            if ((details & Details::W_REQUEST) != 0) {
                requestStream << std::setw(prefixLength) << "Request"
                              << ": " << std::setw(keyLength) << "Method"
                              << " : " << req->method << "\n";
                requestStream << std::setw(prefixLength) << ""
                              << ": " << std::setw(keyLength) << "Url"
                              << " : " << req->url << "\n";
                requestStream << std::setw(prefixLength) << ""
                              << ": " << std::setw(keyLength) << "Version"
                              << " : " << req->httpVersion << "\n";
            }

            std::string prefix;

            if ((details & Details::W_QUERIES) != 0) {
                prefix = "Queries";
                for (const auto& [key, value] : req->queries) {
                    requestStream << std::setw(prefixLength) << prefix << ": " << std::setw(keyLength) << key << " : " << value << "\n";
                    prefix = "";
                }
            }

            if ((details & Details::W_HEADERS) != 0) {
                prefix = "Header";
                for (const auto& [key, value] : req->headers) {
                    requestStream << std::setw(prefixLength) << prefix << ": " << std::setw(keyLength) << key << " : " << value << "\n";
                    prefix = "";
                }
            }

            if ((details & Details::W_COOKIES) != 0) {
                prefix = "Cookies";
                for (const auto& [key, value] : req->cookies) {
                    requestStream << std::setw(prefixLength) << prefix << ": " << std::setw(keyLength) << key << " : " << value << "\n";
                    prefix = "";
                }
            }

            if ((details & Details::W_CONTENT) != 0) {
                if (!req->body.empty()) {
                    prefix = "Body";
                    requestStream << std::setw(prefixLength) << prefix << utils::hexDump(req->body, prefixLength) << "\n";
                }
            }

            std::string requestString = requestStream.str();

            requestString.pop_back();
            LOG(TRACE) << res->getSocketContext()->getSocketConnection()->getInstanceName() << " HTTP: '" << req->method << " " << req->url
                       << " " << req->httpVersion << "'\n"
                       << requestString;

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
