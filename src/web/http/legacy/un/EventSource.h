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

#ifndef WEB_HTTP_LEGACY_UN_EVENTSOURCE_H
#define WEB_HTTP_LEGACY_UN_EVENTSOURCE_H

#include "web/http/client/tools/EventSource.h"
#include "web/http/legacy/un/Client.h"

#include <memory>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::legacy::un {

    class EventSource : public web::http::client::tools::EventSourceT<web::http::legacy::un::Client> {
    private:
        using Super = web::http::client::tools::EventSourceT<web::http::legacy::un::Client>;

    public:
        inline ~EventSource() override;

        friend inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& scheme, //
                                                                                         const SocketAddress& socketAddress,
                                                                                         const std::string& path);
    };

    inline EventSource::~EventSource() {
    }

    inline std::shared_ptr<web::http::client::tools::EventSource>
    EventSource(const std::string& scheme, const net::un::SocketAddress& socketAddress, const std::string& path) {
        std::shared_ptr<class EventSource> eventSource = std::make_shared<class EventSource>();
        eventSource->init(scheme, socketAddress, path);

        return eventSource;
    }

    inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& socketPath, const std::string& path) {
        return EventSource("http", net::un::SocketAddress(socketPath), path);
    }

    inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& url) {
        std::shared_ptr<web::http::client::tools::EventSource> eventSource;

        // One-liner regex: scheme + token (no '/' inside token) + optional /path + optional ?query + optional #fragment
        static const std::regex re(R"(^(?:un://)([^/?#]+)(/[^?#]*)?(\?[^#]*)?(?:#.*)?$)", std::regex::ECMAScript | std::regex::icase);

        auto pct_decode = [](std::string_view s) -> std::string {
            std::string out;
            out.reserve(s.size());
            for (size_t i = 0; i < s.size(); ++i) {
                if (s[i] == '%' && i + 2 < s.size()) {
                    auto hex = [](char c) -> int {
                        if (c >= '0' && c <= '9') {
                            return c - '0';
                        }
                        c |= 0x20;
                        if (c >= 'a' && c <= 'f') {
                            return 10 + (c - 'a');
                        }
                        return -1;
                    };
                    const int h = hex(s[i + 1]);
                    const int l = hex(s[i + 2]);
                    if (h >= 0 && l >= 0) {
                        out.push_back(static_cast<char>((h << 4) | l));
                        i += 2;
                        continue;
                    }
                }
                out.push_back(s[i]);
            }
            return out;
        };

        std::smatch match;
        if (std::regex_match(url, match, re)) {
            // Factory for Unix Domain Sockets (un://)
            // URL form: un://<pct-encoded-sockpath>[/http-path][?query][#fragment]
            //   [1] sockToken -> percent-encoded abs path ("/...") or abstract name ("@name")
            //   [2] path      -> optional HTTP path (defaults to "/")
            //   [3] query     -> optional

            const std::string sockToken = match[1].str();
            std::string socketPath = pct_decode(sockToken); // may start with '/' or '@'

            if ((!socketPath.empty() && (socketPath.front() == '/' || socketPath.front() == '@'))) {
                const std::string scheme = "un"; // fixed
                const std::string httpPath = match[2].matched ? match[2].str() : "/";
                const std::string query = match[3].matched ? match[3].str() : "";

                eventSource = EventSource(scheme, net::un::SocketAddress(socketPath), httpPath + query);
            } else {
                LOG(ERROR) << "UNIX socket must decode to absolute ('/..') or abstract ('@name'): " << sockToken;
            }
        } else {
            LOG(ERROR) << "EventSource unix-domain url not accepted: " << url;
        }

        return eventSource;
    }

} // namespace web::http::legacy::un

#endif // WEB_HTTP_LEGACY_UN_EVENTSOURCE_H
