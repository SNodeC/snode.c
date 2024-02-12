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

#include "express/middleware/VerboseRequest.h"

#include "core/socket/stream/SocketConnection.h"
#include "web/http/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace express::middleware {

    // From: https://gist.github.com/shreyasbharath/32a8092666303a916e24a81b18af146b
    std::string hexDump(const std::vector<uint8_t>& bytes, int prefixLength = 0) {
        std::stringstream hexStream;

        uint8_t buff[17];
        size_t i = 0;

        hexStream << std::hex;

        int currentPrefixLength = 0;

        // Process every byte in the data.
        for (i = 0; i < bytes.size(); i++) {
            // Multiple of 16 means new line (with line offset).

            if ((i % 16) == 0) {
                // Just don't print ASCII for the zeroth line.
                if (i != 0) {
                    hexStream << "  " << buff << std::endl;
                }

                // Output the offset.
                hexStream << Color::Code::FG_BLUE;
                hexStream << std::setw(currentPrefixLength) << std::setfill(' ') << ""
                          << ": " << std::setw(4) << std::setfill('0') << static_cast<unsigned int>(i);
                hexStream << Color::Code::FG_DEFAULT << " ";
                currentPrefixLength = prefixLength;
            }

            // Now the hex code for the specific character.
            hexStream << Color::Code::FG_GREEN;
            hexStream << " " << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(bytes[i]);
            hexStream << Color::Code::FG_DEFAULT;

            // And store a printable ASCII character for later.
            if ((bytes[i] < 0x20) || (bytes[i] > 0x7e)) {
                buff[i % 16] = '.';
            } else {
                buff[i % 16] = bytes[i];
            }
            buff[(i % 16) + 1] = '\0';
        }

        hexStream << std::dec;

        // Pad out last line if not exactly 16 characters.
        while ((i % 16) != 0) {
            hexStream << "   ";
            i++;
        }

        // And print the final ASCII bit.
        hexStream << "  " << buff;

        return hexStream.str();
    }

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
                              << ": " << std::setw(keyLength) << "method"
                              << " : " << req->method << "\n";
                requestStream << std::setw(prefixLength) << ""
                              << ": " << std::setw(keyLength) << "url"
                              << " : " << req->url << "\n";
                requestStream << std::setw(prefixLength) << ""
                              << ": " << std::setw(keyLength) << "http-version"
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
                    requestStream << std::setw(prefixLength) << prefix << hexDump(req->body, prefixLength) << "\n";
                }
            }

            std::string requestString = requestStream.str();
            requestString.pop_back();
            LOG(TRACE) << std::setw(prefixLength) << res->getSocketContext()->getSocketConnection()->getInstanceName() << " HTTP: '"
                       << req->method << " " << req->url << " " << req->httpVersion << "'\n"
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
