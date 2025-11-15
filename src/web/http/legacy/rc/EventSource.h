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

#ifndef WEB_HTTP_LEGACY_RC_EVENTSOURCE_H
#define WEB_HTTP_LEGACY_RC_EVENTSOURCE_H

#include "web/http/client/tools/EventSource.h"
#include "web/http/legacy/rc/Client.h"

#include <memory>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::legacy::rc {

    class EventSource : public web::http::client::tools::EventSourceT<web::http::legacy::rc::Client> {
    private:
        using Super = web::http::client::tools::EventSourceT<web::http::legacy::rc::Client>;

    public:
        inline ~EventSource() override;

        friend inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& scheme, //
                                                                                         const net::rc::SocketAddress& socketAddress,
                                                                                         const std::string& path);
    };

    inline EventSource::~EventSource() {
    }

    inline std::shared_ptr<web::http::client::tools::EventSource>
    EventSource(const std::string& scheme, const net::rc::SocketAddress& socketAddress, const std::string& path) {
        std::shared_ptr<class EventSource> eventSource = std::make_shared<class EventSource>();
        eventSource->init(scheme, socketAddress, path);

        return eventSource;
    }

    inline std::shared_ptr<web::http::client::tools::EventSource>
    EventSource(const std::string& host, uint8_t channel, const std::string& path) {
        return EventSource("http", net::rc::SocketAddress(host, channel), path);
    }

    // Factory for Bluetooth RFCOMM (IPv4-style sibling; rfcomm only)
    inline std::shared_ptr<web::http::client::tools::EventSource> EventSource(const std::string& url) {
        std::shared_ptr<web::http::client::tools::EventSource> eventSource;

        static const std::regex re(
            R"(^(?:rfcomm://)(?![^/]*@)((?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}|[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?(?:\.[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?)*)"
            R"(?::([1-9]|[12]\d|30))?([^?#]*)(\?[^#]*)?(?:#.*)?$)",
            std::regex::ECMAScript | std::regex::icase);

        std::smatch match;
        if (std::regex_match(url, match, re)) {
            // rfcomm://<mac-or-host>[:<channel>][/path][?query][#fragment]
            //  [1] addr   -> MAC "01:23:45:67:89:AB" OR hostname
            //  [2] chan   -> RFCOMM channel 1..30 (optional; default 1)
            //  [3] path   -> may be empty; default "/"
            //  [4] query  -> optional

            const std::string scheme = "rfcomm";     // fixed
            const std::string addr = match[1].str(); // MAC or hostname
            const uint8_t chan = match[2].matched ? static_cast<uint8_t>(std::stoi(match[2].str())) : static_cast<uint16_t>(1);
            const std::string path = match[3].matched ? match[3].str() : "/";
            const std::string query = match[4].matched ? match[4].str() : "";

            eventSource = EventSource(scheme, net::rc::SocketAddress(addr, chan), path + query);
        } else {
            LOG(ERROR) << "EventSource RFCOMM url not accepted: " << url;
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

        return EventSource(!origin.empty() ? origin + path : "");
    }

} // namespace web::http::legacy::rc

#endif // WEB_HTTP_LEGACY_RC_EVENTSOURCE_H
