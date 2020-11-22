/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <filesystem>
#include <numeric>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Response.h"
#include "ServerContext.h"
#include "StatusCodes.h"
#include "file/FileReader.h"
#include "http_utils.h"

namespace http {

    Response::Response(ServerContextBase* serverContext)
        : serverContext(serverContext) {
    }

    void Response::enqueue(const char* buf, size_t len) {
        if (!headersSent && !sendHeaderInProgress) {
            sendHeaderInProgress = true;
            sendHeader();
            sendHeaderInProgress = false;
            headersSent = true;
        }

        serverContext->sendResponseData(buf, len);

        if (headersSent) {
            contentSent += len;
            if (contentSent == contentLength) {
                serverContext->responseCompleted();
            } else if (contentSent > contentLength) {
                serverContext->terminateConnection();
            }
        }
    }

    void Response::enqueue(const std::string& str) {
        enqueue(str.c_str(), str.size());
    }

    Response& Response::status(int status) {
        responseStatus = status;

        return *this;
    }

    Response& Response::append(const std::string& field, const std::string& value) {
        std::map<std::string, std::string>::iterator it = headers.find(field);

        if (it != headers.end()) {
            it->second += ", " + value;
        } else {
            set(field, value);
        }

        return *this;
    }

    Response& Response::set(const std::map<std::string, std::string>& headers, bool overwrite) {
        for (auto& [field, value] : headers) {
            set(field, value, overwrite);
        }

        return *this;
    }

    Response& Response::set(const std::string& field, const std::string& value, bool overwrite) {
        if (overwrite) {
            headers.insert_or_assign(field, value);
        } else {
            headers.insert({field, value});
        }

        if (field == "Content-Length") {
            contentLength = std::stol(value);
        } else if (field == "Connection" && httputils::ci_comp(value, "close")) {
            connectionState = ConnectionState::Close;
        } else if (field == "Connection" && httputils::ci_comp(value, "keep-alive")) {
            connectionState = ConnectionState::Keep;
        }

        return *this;
    }

    Response& Response::type(const std::string& type) {
        set({{"Content-Type", type}});

        return *this;
    }

    Response& Response::cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options) {
        cookies.insert({name, CookieOptions(value, options)});

        return *this;
    }

    Response& Response::clearCookie(const std::string& name, const std::map<std::string, std::string>& options) {
        std::map<std::string, std::string> opts = options;

        opts.erase("Max-Age");
        time_t time = 0;
        opts["Expires"] = httputils::to_http_date(gmtime(&time));

        cookie(name, "", opts);

        return *this;
    }

    void Response::send(const char* buffer, size_t size) {
        if (size > 0) {
            headers.insert({"Content-Type", "application/octet-stream"});
        }
        headers.insert_or_assign("Content-Length", std::to_string(size));

        enqueue(buffer, size);
    }

    void Response::send(const std::string& text) {
        if (text.size() > 0) {
            headers.insert({"Content-Type", "text/html; charset=utf-8"});
        }
        send(text.c_str(), text.size());
    }

    void Response::sendHeader() {
        enqueue("HTTP/1.1 " + std::to_string(responseStatus) + " " + StatusCode::reason(responseStatus) + "\r\n");
        enqueue("Date: " + httputils::to_http_date() + "\r\n");

        headers.insert({{"Cache-Control", "public, max-age=0"}, {"Accept-Ranges", "bytes"}, {"X-Powered-By", "snode.c"}});

        for (auto& [field, value] : headers) {
            enqueue(field + ": " + value + "\r\n");
        }

        for (auto& [cookie, cookieValue] : cookies) {
            std::string cookieString =
                std::accumulate(cookieValue.getOptions().begin(),
                                cookieValue.getOptions().end(),
                                cookie + "=" + cookieValue.getValue(),
                                [](const std::string& str, const std::pair<const std::string&, const std::string&> option) -> std::string {
                                    return str + "; " + option.first + (!option.second.empty() ? "=" + option.second : "");
                                });
            enqueue("Set-Cookie: " + cookieString + "\r\n");
        }

        enqueue("\r\n");

        if (headers.find("Content-Length") != headers.end()) {
            contentLength = std::stoi(headers.find("Content-Length")->second);
        } else {
            contentLength = 0;
        }
    }

    void Response::end() {
        send("");
    }

    void Response::reset() {
        headersSent = false;
        sendHeaderInProgress = false;
        contentSent = 0;
        responseStatus = 200;
        contentLength = 0;
        connectionState = ConnectionState::Default;
        headers.clear();
        cookies.clear();
    }

} // namespace http
