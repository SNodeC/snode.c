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

#include "web/http/client/Request.h"

#include "web/http/SocketContext.h"
#include "web/http/client/SocketContextUpgradeFactorySelector.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility>

// IWYU pragma: no_include <bits/utility.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    Request::Request(web::http::SocketContext* clientContext)
        : socketContext(clientContext) {
    }

    Request& Request::setHost(const std::string& host) {
        this->host = host;

        return *this;
    }

    Request& Request::append(const std::string& field, const std::string& value) {
        std::map<std::string, std::string>::iterator it = headers.find(field);

        if (it != headers.end()) {
            it->second += ", " + value;
        } else {
            set(field, value);
        }

        return *this;
    }

    Request& Request::set(const std::map<std::string, std::string>& headers, bool overwrite) {
        for (auto& [field, value] : headers) {
            set(field, value, overwrite);
        }

        return *this;
    }

    Request& Request::set(const std::string& field, const std::string& value, bool overwrite) {
        if (overwrite) {
            headers.insert_or_assign(field, value);
        } else {
            headers.insert({field, value});
        }

        if (field == "Content-Length") {
            contentLength = std::stoul(value);
        } else if (field == "Connection" && httputils::ci_contains(value, "close")) {
            connectionState = ConnectionState::Close;
        } else if (field == "Connection" && httputils::ci_contains(value, "keep-alive")) {
            connectionState = ConnectionState::Keep;
        }

        return *this;
    }

    Request& Request::cookie(const std::string& name, const std::string& value) {
        cookies.insert({name, value});

        return *this;
    }

    Request& Request::cookie(const std::map<std::string, std::string>& cookies) {
        for (auto& [name, value] : cookies) {
            cookie(name, value);
        }
        return *this;
    }

    Request& Request::type(const std::string& type) {
        headers.insert({"Content-Type", type});

        return *this;
    }

    void Request::reset() {
        method = "GET";
        url.clear();
        httpMajor = 1;
        httpMinor = 1;
        host.clear();
        queries.clear();
        headers.clear();
        cookies.clear();
        headersSent = false;
        sendHeaderInProgress = false;
        contentLength = 0;
        contentSent = 0;

        connectionState = ConnectionState::Default;
    }

    void Request::enqueue(const char* junk, std::size_t junkLen) {
        if (!headersSent && !sendHeaderInProgress) {
            sendHeaderInProgress = true;
            sendHeader();
            sendHeaderInProgress = false;
            headersSent = true;
        }

        socketContext->sendToPeer(junk, junkLen);

        if (headersSent) {
            contentSent += junkLen;
            if (contentSent == contentLength) {
                socketContext->sendToPeerCompleted();
            } else if (contentSent > contentLength) {
                socketContext->close();
            }
        }
    }

    void Request::enqueue(const std::string& data) {
        enqueue(data.data(), data.size());
    }

    void Request::sendHeader() {
        std::string httpVersion = "HTTP/" + std::to_string(httpMajor) + "." + std::to_string(httpMinor);

        std::string queryString = "";
        if (queries.size() > 0) {
            queryString += "?";
            for (auto& [key, value] : queries) {
                queryString += httputils::url_encode(key) + "=" + httputils::url_encode(value) + "&";
            }
            queryString.pop_back();
        }

        enqueue(method + " " + url + queryString + " " + httpVersion + "\r\n");

        if (!host.empty()) {
            enqueue("Host: " + host + "\r\n");
        }
        enqueue("Date: " + httputils::to_http_date() + "\r\n");

        headers.insert({{"Cache-Control", "public, max-age=0"}, {"Accept-Ranges", "bytes"}, {"X-Powered-By", "snode.c"}});

        for (const auto& [field, value] : headers) {
            enqueue(field + ":" + value + "\r\n");
        }

        if (contentLength != 0) {
            enqueue("Content-Length: " + std::to_string(contentLength) + "\r\n");
        }

        for (const auto& [name, value] : cookies) {
            enqueue("Cookie:" + name + "=" + value + "\r\n");
        }

        enqueue("\r\n");

        if (headers.find("Content-Length") != headers.end()) {
            contentLength = std::stoul(headers.find("Content-Length")->second);
        } else {
            contentLength = 0;
        }
    }

    void Request::send(const char* junk, std::size_t junkLen) {
        if (junkLen > 0) {
            headers.insert({"Content-Type", "application/octet-stream"});
        }
        headers.insert_or_assign("Content-Length", std::to_string(junkLen));

        enqueue(junk, junkLen);
    }

    void Request::send(const std::string& junk) {
        if (junk.size() > 0) {
            headers.insert({"Content-Type", "text/html; charset=utf-8"});
        }
        send(junk.data(), junk.size());
    }

    void Request::upgrade(const std::string& url, const std::string& protocol) {
        this->url = url;

        set("Connection", "Upgrade", true);
        set("Upgrade", protocol);

        // load upgrade context
        // let upgrade context fill the request-header fields
        // send the request to the server
        // the response-code needs to check the response from the server and upgrade the context in case the server says OK

        web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory =
            web::http::client::SocketContextUpgradeFactorySelector::instance()->select(protocol, *this);

        if (socketContextUpgradeFactory != nullptr) {
            start();
            socketContextUpgradeFactory->checkRefCount();
        } else {
            socketContext->close();
        }
    }

    void Request::start() {
        send("");
    }

    const std::string& Request::header(const std::string& field) {
        return headers[field];
    }

} // namespace web::http::client
