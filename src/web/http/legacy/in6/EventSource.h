/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef WEB_HTTP_LEGACY_IN6_EVENTSOURCE_H
#define WEB_HTTP_LEGACY_IN6_EVENTSOURCE_H

#include "web/http/client/tools/EventSource.h"
#include "web/http/legacy/in6/Client.h"

#include <memory>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::legacy::in6 {

    class EventSource : public web::http::client::tools::EventSourceT<web::http::legacy::in6::Client> {
    private:
        using Super = web::http::client::tools::EventSourceT<web::http::legacy::in6::Client>;

    public:
        inline ~EventSource() override;

        friend inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& scheme, //
                                                                                         const net::in6::SocketAddress& socketAddress,
                                                                                         const std::string& path);
    };

    inline EventSource::~EventSource() {
    }

    inline std::shared_ptr<web::http::client::tools::EventSource>
    EventSource(const std::string& scheme, const net::in6::SocketAddress& socketAddress, const std::string& path) {
        std::shared_ptr<class EventSource> eventSource = std::make_shared<class EventSource>();
        eventSource->init(scheme, socketAddress, path);

        return eventSource;
    }

    inline std::shared_ptr<web::http::client::tools::EventSource>
    EventSource(const std::string& host, uint16_t port, const std::string& path) {
        return EventSource("http", net::in6::SocketAddress(host, port), path);
    }

    inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& url) {
        std::shared_ptr<web::http::client::tools::EventSource> eventSource;

        static const std::regex re(
            R"(^(?:(https?)://)?(?![^/]*@)((?:(?:25[0-5]|2[0-4]\d|1?\d{1,2})\.){3}(?:25[0-5]|2[0-4]\d|1?\d{1,2})|[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?(?:\.[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?)*)(?::([0-9]{1,5}))?([^?#]*)(\?[^#]*)?(?:#.*)?$)",
            std::regex::ECMAScript | std::regex::icase);

        std::smatch match;
        if (std::regex_match(url, match, re)) {
            //  [1] scheme    (optional) -> "http" or "https"
            //  [2] host      (required) -> hostname or IPv4 token (no [ ], no :)
            //  [3] port      (optional) -> digits
            //  [4] path      (may be empty)
            //  [5] query     (optional)

            const std::string& scheme = match[1].matched ? match[1].str() : "http";
            const std::string& host = match[2].str();
            const uint16_t port = match[3].matched ? static_cast<uint16_t>(std::stoi(match[3].str())) : static_cast<uint16_t>(80);
            const std::string& path = (match[4].matched ? match[4].str() : "/");
            const std::string& query = (match[5].matched ? match[5].str() : "");

            if (scheme == "http") {
                eventSource = EventSource(scheme, net::in6::SocketAddress(host, port), path + query);
            } else {
                LOG(ERROR) << "Scheme not valid: " << scheme;
            }
        } else {
            LOG(ERROR) << "EventSource url not accepted: " << url;
        }

        return eventSource;
    }

    inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(std::string origin, std::string path) {
        if (!origin.empty() && origin.at(origin.length() - 1) == '/') {
            origin.pop_back();
        }

        if (!path.empty() && path.at(0) != '/') {
            path = "/" + path;
        }

        return EventSource(!origin.empty() ? origin + path : path);
    }

} // namespace web::http::legacy::in6

#endif // WEB_HTTP_LEGACY_IN6_EVENTSOURCE_H
