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
#include "web/http/StatusCodes.h"
#include "web/http/http_utils.h"
#include "web/http/server/Request.h"
#include "web/http/server/RequestContextBase.h"
#include "web/http/server/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/system/time.h"

#include <cerrno>
#include <filesystem>
#include <numeric>
#include <system_error>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    Response::Response(RequestContextBase* requestContext)
        : requestContext(requestContext) {
    }

    Response::~Response() {
        if (fileReader != nullptr) {
            delete fileReader;
        }
    }

    void Response::sendResponse(const char* junk, std::size_t junkLen) {
        if (!headersSent && !sendHeaderInProgress) {
            sendHeaderInProgress = true;
            sendHeader();
            sendHeaderInProgress = false;
            headersSent = true;
        }

        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
        requestContext->sendToPeer(junk, junkLen);

        if (headersSent) {
            contentSent += junkLen;
        }
    }

    void Response::sendResponse(const std::string& junk) {
        sendResponse(junk.data(), junk.size());
    }

    void Response::send(const char* junk, std::size_t junkLen) {
        if (junkLen > 0) {
            set("Content-Type", "application/octet-stream", false);
        }
        set("Content-Length", std::to_string(junkLen), false);

        sendResponse(junk, junkLen);

        sendToPeerCompleted();
    }

    void Response::send(const std::string& junk) {
        if (!junk.empty()) {
            set("Content-Type", "text/html; charset=utf-8", false);
        }
        set("Content-Length", std::to_string(junk.size()), false);

        send(junk.data(), junk.size());
    }

    void Response::stream(core::file::FileReader* fileReader) {
        requestContext->streamToPeer(fileReader);

        // Set Header correctly
        sendResponse("");
    }

    void Response::end() {
        send("");
    }

    Response& Response::status(int status) {
        responseStatus = status;

        return *this;
    }

    Response& Response::append(const std::string& field, const std::string& value) {
        const std::map<std::string, std::string>::iterator it = headers.find(field);

        if (it != headers.end()) {
            it->second += ", " + value;
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
        const time_t time = 0;
        opts["Expires"] = httputils::to_http_date(utils::system::gmtime(&time));

        return cookie(name, "", opts);
    }

    /* Just an UML-Sequencediagram test */

    /** Sequence diagramm of res.upgrade(req).
@startuml
!include web/http/server/pu/response_upgrade.pu
@enduml
     */

    [[nodiscard]] bool Response::upgrade(Request& req) {
        bool success = false;

        if (httputils::ci_contains(req.get("connection"), "Upgrade")) {
            web::http::server::SocketContextUpgradeFactory* socketContextUpgradeFactory =
                web::http::server::SocketContextUpgradeFactorySelector::instance()->select(req, *this);

            if (socketContextUpgradeFactory != nullptr) {
                success = requestContext->switchSocketContext(socketContextUpgradeFactory);
            } else {
                set("Connection", "close").status(404);
            }
        } else {
            set("Connection", "close").status(400);
        }

        return success;
    }

    void Response::sendFile(const std::string& file, const std::function<void(int errnum)>& onError) {
        std::string absolutFileName = file;

        if (std::filesystem::exists(absolutFileName)) {
            std::error_code ec;
            absolutFileName = std::filesystem::canonical(absolutFileName);

            if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                fileReader = core::file::FileReader::open(absolutFileName, *this, [this, &absolutFileName, onError](int err) -> void {
                    if (err == 0) {
                        headers.insert({{"Content-Type", web::http::MimeTypes::contentType(absolutFileName)},
                                        {"Last-Modified", httputils::file_mod_http_date(absolutFileName)}});
                        headers.insert_or_assign("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName)));
                    } else {
                        onError(err);
                    }
                });

                stream(fileReader);
            } else {
                errno = EEXIST;
                onError(errno);
            }
        } else {
            errno = ENOENT;
            onError(errno);
        }
    }

    void Response::sendToPeerCompleted() {
        VLOG(0) << "&&&&&&&&&&&&&&&&&&&&& 1: " << contentSent << " - " << contentLength;
        if (contentSent == contentLength) {
            VLOG(0) << "&&&&&&&&&&&&&&&&&&&&& 2";
            requestContext->sendToPeerCompleted();
        } else if (contentSent > contentLength) {
            requestContext->close();
        }

        delete requestContext;
    }

    void Response::sendHeader() {
        sendResponse("HTTP/1.1 " + std::to_string(responseStatus) + " " + StatusCode::reason(responseStatus) + "\r\n");
        sendResponse("Date: " + httputils::to_http_date() + "\r\n");

        set({{"Cache-Control", "public, max-age=0"}, {"Accept-Ranges", "bytes"}, {"X-Powered-By", "snode.c"}});

        for (const auto& [field, value] : headers) {
            sendResponse(std::string(field).append(": ").append(value).append("\r\n"));
        }

        for (const auto& [cookie, cookieValue] : cookies) { // cppcheck-suppress shadowFunction
            const std::string cookieString =
                std::accumulate(cookieValue.getOptions().begin(),
                                cookieValue.getOptions().end(),
                                cookie + "=" + cookieValue.getValue(),
                                [](const std::string& str, const std::pair<const std::string&, const std::string&> option) -> std::string {
                                    return str + "; " + option.first + (!option.second.empty() ? "=" + option.second : "");
                                });
            sendResponse("Set-Cookie: " + cookieString + "\r\n");
        }

        sendResponse("\r\n");

        if (headers.find("Content-Length") != headers.end()) {
            contentLength = std::stoul(headers.find("Content-Length")->second);
        } else {
            contentLength = 0;
        }
    }

    void Response::receive(const char* junk, std::size_t junkLen) {
        sendResponse(junk, junkLen);
    }

    void Response::eof() {
        VLOG(0) << "Response: FileReader EOF";
        delete fileReader;
        fileReader = nullptr;

        sendToPeerCompleted();
    }

    void Response::error(int errnum) {
        VLOG(0) << "Response: FileReader ERROR";
        errno = errnum;

        requestContext->close();

        delete fileReader;
        fileReader = nullptr;

        sendToPeerCompleted();
    }

} // namespace web::http::server
