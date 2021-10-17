/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "web/http/server/Response.h"

#include "log/Logger.h"
#include "net/system/time.h"
#include "web/http/StatusCodes.h"
#include "web/http/http_utils.h"
#include "web/http/server/Request.h"
#include "web/http/server/SocketContext.h"
#include "web/http/server/SocketContextUpgradeFactory.h"
#include "web/http/server/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <numeric>
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    Response::Response(SocketContext* serverContext)
        : socketContext(serverContext) {
    }

    void Response::enqueue(const char* junk, std::size_t junkLen) {
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
                socketContext->terminateConnection();
            }
        }
    }

    void Response::enqueue(const std::string& junk) {
        enqueue(junk.data(), junk.size());
    }

    void Response::send(const char* junk, std::size_t junkLen) {
        if (junkLen > 0) {
            set("Content-Type", "application/octet-stream", false);
        }
        set("Content-Length", std::to_string(junkLen));

        enqueue(junk, junkLen);
    }

    void Response::send(const std::string& junk) {
        if (junk.size() > 0) {
            set("Content-Type", "text/html; charset=utf-8");
        }
        send(junk.data(), junk.size());
    }

    void Response::end() {
        send("");
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
        if (overwrite || headers.find(field) == headers.end()) {
            headers.insert_or_assign(field, value);

            if (field == "Content-Length") {
                contentLength = std::stoul(value);
            } else if (field == "Connection" && httputils::ci_contains(value, "close")) {
                connectionState = ConnectionState::Close;
            } else if (field == "Connection" && httputils::ci_contains(value, "keep-alive")) {
                connectionState = ConnectionState::Keep;
            }
        }

        return *this;
    }

    Response& Response::type(const std::string& type) {
        return set("Content-Type", type);
    }

    Response& Response::cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options) {
        cookies.insert({name, CookieOptions(value, options)});

        return *this;
    }

    Response& Response::clearCookie(const std::string& name, const std::map<std::string, std::string>& options) {
        std::map<std::string, std::string> opts = options;

        opts.erase("Max-Age");
        time_t time = 0;
        opts["Expires"] = httputils::to_http_date(net::system::gmtime(&time));

        return cookie(name, "", opts);
    }

    /* Just an UML-Sequencediagram test */

    /** Sender class. Can be used to send a command to the server.
     *  The receiver will acknowledge the command by calling Ack().
     *  \startuml
     *    Response->SocketContextUpgradeFactorySelector  : select(request, *this)
     *    Response<--SocketContextUpgradeFactorySelector : socketContextUpgradeFactory
     *    Response->SocketContext : switchSocketContext(socketContextUpgradeFactory)
     *    SocketContext->SocketConnection : switchSocketContext(socketContextUpgradeFactory)
     *    SocketConnection->SocketContextUpgradeFactory : create(this)
     *    SocketConnection<--SocketContextUpgradeFactory : NewSocketContext
     *    SocketConnection->SocketContext : onDisconnected()
     *    SocketConnection<--SocketContext
     *    SocketConnection->SocketConnection : SocketContext=NewSocketContext
     *    SocketConnection->SocketContext : onConnected()
     *    SocketConnection<--SocketContext
     *    SocketContext<--SocketConnection
     *    Response<--SocketContext
     *  \enduml
     */

    void Response::upgrade(Request& req) {
        if (httputils::ci_contains(req.header("connection"), "Upgrade")) {
            web::http::server::SocketContextUpgradeFactory* socketContextUpgradeFactory =
                web::http::server::SocketContextUpgradeFactorySelector::instance()->select(req, *this);

            if (socketContextUpgradeFactory != nullptr) {
                if (socketContext->switchSocketContext(socketContextUpgradeFactory) == nullptr) {
                }
            } else {
                set("Connection", "close");
                this->status(404).end();
            }

        } else {
            set("Connection", "close");
            this->status(400).end();
        }
    }

    void Response::sendHeader() {
        enqueue("HTTP/1.1 " + std::to_string(responseStatus) + " " + StatusCode::reason(responseStatus) + "\r\n");
        enqueue("Date: " + httputils::to_http_date() + "\r\n");

        set({{"Cache-Control", "public, max-age=0"}, {"Accept-Ranges", "bytes"}, {"X-Powered-By", "snode.c"}});

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
            contentLength = std::stoul(headers.find("Content-Length")->second);
        } else {
            contentLength = 0;
        }
    }

    void Response::receive(const char* junk, std::size_t junkLen) {
        enqueue(junk, junkLen);
    }

    void Response::eof() {
        LOG(INFO) << "Stream EOF";
    }

    void Response::error([[maybe_unused]] int errnum) {
        PLOG(ERROR) << "Stream error: ";
        socketContext->terminateConnection();
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

} // namespace web::http::server
