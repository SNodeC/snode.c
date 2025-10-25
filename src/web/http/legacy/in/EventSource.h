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

#ifndef WEB_HTTP_LEGACY_IN_EVENTSOURCE_H
#define WEB_HTTP_LEGACY_IN_EVENTSOURCE_H

#include "web/http/client/tools/EventSource.h"
#include "web/http/legacy/in/Client.h"

#include <memory>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::legacy::in {

    class EventSource : public web::http::client::tools::EventSourceT<web::http::legacy::in::Client> {
    private:
        using Super = web::http::client::tools::EventSourceT<web::http::legacy::in::Client>;

    public:
        inline ~EventSource() override;

        friend inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& url);
        friend inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(std::string origin, std::string path);
        friend inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const net::in::SocketAddress& socketAddress, //
                                                                                         const std::string& path);
        friend inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& host, //
                                                                                         uint16_t port,
                                                                                         const std::string& path);
    };

    inline EventSource::~EventSource() {
    }

    inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& url) {
        std::shared_ptr<class EventSource> eventSource;

        static const std::regex re(R"(^((https?)://)?([^/:]+)(?::(\d+))?(/.*)?$)");
        std::smatch match;
        if (std::regex_match(url, match, re)) {
            const std::string& host = match[3].str();
            const uint16_t port = match[4].matched ? static_cast<uint16_t>(std::stoi(match[4].str())) : static_cast<uint16_t>(80);
            const std::string& path = (match[5].matched ? match[5].str() : "/");

            eventSource = std::make_shared<class EventSource>();
            eventSource->init("http", net::in::SocketAddress(host, port), path);
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

        return EventSource(!origin.empty() && !path.empty() ? origin + path : "");
    }

    inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const net::in::SocketAddress& socketAddress,
                                                                              const std::string& path) {
        std::shared_ptr<class EventSource> eventSource = std::make_shared<class EventSource>();
        eventSource->init("http", socketAddress, path);

        return eventSource;
    }

    inline std::shared_ptr<web::http::client::tools::EventSource>
    EventSource(const std::string& host, uint16_t port, const std::string& path) {
        return EventSource(net::in::SocketAddress(host, port), path);
    }

} // namespace web::http::legacy::in

#endif // WEB_HTTP_LEGACY_IN_EVENTSOURCE_H
