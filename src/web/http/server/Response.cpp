/*
 * SNode.C - a slim toolkit for network communication
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

#include "web/http/server/Response.h"

#include "core/file/FileReader.h"
#include "web/http/MimeTypes.h"
#include "web/http/SocketContext.h"
#include "web/http/StatusCodes.h"
#include "web/http/http_utils.h"
#include "web/http/server/Request.h"
#include "web/http/server/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/system/time.h"

#include <cerrno>
#include <filesystem>
#include <numeric>
#include <system_error>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    Response::Response(SocketContext* socketContext)
        : socketContext(socketContext) {
    }

    Response::~Response() {
        if (socketContext != nullptr) {
            socketContext->streamEof();
        }
    }

    void Response::reInit() {
        statusCode = 200;
        headers.clear();
        cookies.clear();
        contentLength = 0;
        contentSent = 0;
        connectionState = ConnectionState::Default;
    }

    Response& Response::status(int status) {
        statusCode = status;

        return *this;
    }

    Response& Response::append(const std::string& field, const std::string& value) {
        const std::map<std::string, std::string>::iterator it = headers.find(field);

        if (it != headers.end()) {
            set(field, it->second.append(", ").append(value));
        } else {
            set(field, value);
        }

        return *this;
    }

    Response& Response::set(const std::map<std::string, std::string>& headers, bool overwrite) {
        for (const auto& [field, value] : headers) {
            set(field, value, overwrite);
        }

        return *this;
    }

    Response& Response::set(const std::string& field, const std::string& value, bool overwrite) {
        if (!value.empty()) {
            if (overwrite || headers.find(field) == headers.end()) {
                headers.insert_or_assign(field, value);

                if (httputils::ci_contains(field, "Content-Length")) {
                    contentLength = std::stoul(value);
                } else if (httputils::ci_contains(field, "Connection") && httputils::ci_contains(value, "close")) {
                    connectionState = ConnectionState::Close;
                } else if (httputils::ci_contains(field, "Connection") && httputils::ci_contains(value, "keep-alive")) {
                    connectionState = ConnectionState::Keep;
                }
            }
        } else {
            headers.erase(field);
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
        const time_t time = 0;
        opts["Expires"] = httputils::to_http_date(utils::system::gmtime(&time));

        return cookie(name, "", opts);
    }

    void Response::send(const char* junk, std::size_t junkLen) {
        if (junkLen > 0) {
            set("Content-Type", "application/octet-stream", false);
        }
        set("Content-Length", std::to_string(junkLen));

        sendHeader();
        sendFragment(junk, junkLen);
        sendCompleted();
    }

    void Response::send(const std::string& junk) {
        if (!junk.empty()) {
            set("Content-Type", "text/html; charset=utf-8", false);
        }

        send(junk.data(), junk.size());
    }

    /* Just an UML-Sequence diagram test */
    /** Sequence diagram of res.upgrade(req).
@startuml
!include web/http/server/pu/response_upgrade.pu
@enduml
     */
    void Response::upgrade(std::shared_ptr<Request> request, const std::function<void(bool)>& status) {
        if (socketContext != nullptr) {
            if (httputils::ci_contains(request->get("connection"), "Upgrade")) {
                web::http::server::SocketContextUpgradeFactory* socketContextUpgradeFactory =
                    web::http::server::SocketContextUpgradeFactorySelector::instance()->select(*request, *this);

                if (socketContextUpgradeFactory != nullptr) {
                    socketContextUpgrade = socketContextUpgradeFactory->create(socketContext->getSocketConnection());
                    if (socketContextUpgrade == nullptr) {
                        set("Connection", "close").status(404);
                    }
                } else {
                    set("Connection", "close").status(404);
                }
            } else {
                set("Connection", "close").status(400);
            }
        }

        end();

        status(socketContextUpgrade != nullptr);
    }

    void Response::end() {
        send("");
    }

    void Response::sendFile(const std::string& file, const std::function<void(int errnum)>& callback) {
        std::string absolutFileName = file;

        if (std::filesystem::exists(absolutFileName)) {
            std::error_code ec;
            absolutFileName = std::filesystem::canonical(absolutFileName);

            if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                core::file::FileReader::open(absolutFileName)
                    ->pipe(this, [this, &absolutFileName, &callback](core::pipe::Source* source, int errnum) -> void {
                        callback(errnum);

                        if (errnum == 0) {
                            set("Content-Type", web::http::MimeTypes::contentType(absolutFileName), false);
                            set("Last-Modified", httputils::file_mod_http_date(absolutFileName), false);
                            set("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName)));
                        } else {
                            source->stop();
                        }
                    });
            } else {
                errno = EEXIST;
                callback(errno);
            }
        } else {
            errno = ENOENT;
            callback(errno);
        }
    }

    void Response::stopResponse() {
        stop();
        socketContext = nullptr;
    }

    Response& Response::sendHeader() {
        socketContext->sendToPeer("HTTP/1.1 " + std::to_string(statusCode) + " " + StatusCode::reason(statusCode) + "\r\n");
        socketContext->sendToPeer("Date: " + httputils::to_http_date() + "\r\n");

        set("X-Powered-By", "snode.c");

        for (const auto& [field, value] : headers) {
            socketContext->sendToPeer(std::string(field).append(": ").append(value).append("\r\n"));
        }

        for (const auto& [cookie, cookieValue] : cookies) { // cppcheck-suppress shadowFunction
            const std::string cookieString =
                std::accumulate(cookieValue.getOptions().begin(),
                                cookieValue.getOptions().end(),
                                cookie + "=" + cookieValue.getValue(),
                                [](const std::string& str, const std::pair<const std::string&, const std::string&> option) -> std::string {
                                    return str + "; " + option.first + (!option.second.empty() ? "=" + option.second : "");
                                });
            socketContext->sendToPeer("Set-Cookie: " + cookieString + "\r\n");
        }

        socketContext->sendToPeer("\r\n");

        return *this;
    }

    Response& Response::sendFragment(const char* junk, std::size_t junkLen) {
        if (socketContext != nullptr) {
            socketContext->sendToPeer(junk, junkLen);
            contentSent += junkLen;
        }

        return *this;
    }

    Response& Response::sendFragment(const std::string& junk) {
        return sendFragment(junk.data(), junk.size());
    }

    void Response::sendCompleted() {
        if (socketContext != nullptr) {
            if (socketContextUpgrade != nullptr) {
                socketContext->switchSocketContext(socketContextUpgrade);
            }

            socketContext->sendToPeerCompleted(contentSent == contentLength);
        }
    }

    void Response::onSourceConnect(core::pipe::Source* source) {
        if (socketContext != nullptr && socketContext->streamToPeer(source)) {
            sendHeader();

            source->start();
        } else {
            source->stop();
        }
    }

    void Response::onSourceData(const char* junk, std::size_t junkLen) {
        sendFragment(junk, junkLen);
    }

    void Response::onSourceEof() {
        if (socketContext != nullptr) {
            socketContext->streamEof();
        }

        sendCompleted();
    }

    void Response::onSourceError(int errnum) {
        errno = errnum;

        if (socketContext != nullptr) {
            socketContext->streamEof();
            socketContext->close();
        }

        sendCompleted();
    }

    const std::string& Response::header(const std::string& field) {
        return headers[field];
    }

    SocketContext* Response::getSocketContext() const {
        return socketContext;
    }

} // namespace web::http::server
