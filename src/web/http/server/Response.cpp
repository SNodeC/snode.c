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

#include "web/http/server/Response.h"

#include "SocketContext.h"
#include "core/file/FileReader.h"
#include "core/socket/stream/SocketConnection.h"
#include "web/http/MimeTypes.h"
#include "web/http/StatusCodes.h"
#include "web/http/server/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/system/time.h"
#include "web/http/CiStringMap.h"
#include "web/http/http_utils.h"

#include <cerrno>
#include <filesystem>
#include <iterator>
#include <numeric>
#include <sstream>
#include <system_error>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define to_hex_str(int_val) (static_cast<std::ostringstream const&>(std::ostringstream() << std::uppercase << std::hex << int_val)).str()

namespace web::http::server {

    Response::Response(SocketContext* socketContext)
        : socketContext(socketContext) {
    }

    Response::~Response() {
        if (socketContext != nullptr && Sink::isStreaming()) {
            socketContext->streamEof();
        }
    }

    void Response::stopResponse() {
        stop();
    }

    void Response::init() {
        statusCode = 200;
        httpMajor = 1;
        httpMinor = 1;
        headers.clear();
        cookies.clear();
        trailer.clear();
        contentLength = 0;
        contentSent = 0;
        connectionState = ConnectionState::Default;
        transferEncoding = TransferEncoding::HTTP10;
    }

    Response& Response::status(int statusCode) {
        this->statusCode = statusCode;

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
            if (overwrite) {
                headers.insert_or_assign(field, value);
            } else {
                headers.insert({field, value});
            }

            if (web::http::ciEquals(field, "Connection")) {
                if (web::http::ciContains(headers[field], "keep-alive")) {
                    connectionState = ConnectionState::Keep;
                } else if (web::http::ciContains(headers[field], "close")) {
                    connectionState = ConnectionState::Close;
                }
            } else if (web::http::ciEquals(field, "Content-Length")) {
                contentLength = std::stoul(value);
                transferEncoding = TransferEncoding::Identity;
                headers.erase("Transfer-Encoding");
            } else if (web::http::ciEquals(field, "Transfer-Encoding")) {
                if (web::http::ciContains(headers[field], "chunked")) {
                    transferEncoding = TransferEncoding::Chunked;
                    headers.erase("Content-Length");
                }
                if (web::http::ciContains(headers[field], "compressed")) {
                }
                if (web::http::ciContains(headers[field], "deflate")) {
                }
                if (web::http::ciContains(headers[field], "gzip")) {
                }
            } else if (web::http::ciEquals(field, "Content-Encoding")) {
                if (web::http::ciContains(headers[field], "compressed")) {
                }
                if (web::http::ciContains(headers[field], "deflate")) {
                }
                if (web::http::ciContains(headers[field], "gzip")) {
                }
                if (web::http::ciContains(headers[field], "br")) {
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

    Response& Response::setTrailer(const std::string& field, const std::string& value, bool overwrite) {
        if (!value.empty()) {
            if (overwrite) {
                trailer.insert_or_assign(field, value);
            } else {
                trailer.insert({field, value});
            }
            if (!headers.contains("Trailer")) {
                set("Trailer", field);
            } else {
                headers["Trailer"].append("," + field);
            }
        } else {
            trailer.erase(field);
        }

        return *this;
    }

    void Response::send(const char* chunk, std::size_t chunkLen) {
        if (chunkLen > 0) {
            set("Content-Type", "application/octet-stream", false);
        }
        set("Content-Length", std::to_string(chunkLen));

        sendHeader();
        sendFragment(chunk, chunkLen);
        sendCompleted();
    }

    void Response::send(const std::string& chunk) {
        if (!chunk.empty()) {
            set("Content-Type", "text/html; charset=utf-8", false);
        }

        send(chunk.data(), chunk.size());
    }

    void Response::sendStatus(int statusCode) {
        status(statusCode);

        send(StatusCode::reason(statusCode));
    }

    /* Just an UML-Sequence diagram test */
    /** Sequence diagram of res.upgrade(req).
@startuml
!include web/http/server/pu/response_upgrade.pu
@enduml
     */
    void Response::upgrade(const std::shared_ptr<Request>& request, const std::function<void(const std::string&)>& status) {
        const std::string connectionName = socketContext->getSocketConnection()->getConnectionName();

        std::string name;

        LOG(DEBUG) << connectionName << " HTTP: Initiating upgrade: " << request->method << " " << request->url
                   << " HTTP/" + std::to_string(httpMajor) + "." + std::to_string(httpMinor) << "\n"
                   << httputils::toString(request->method,
                                          request->url,
                                          "HTTP/" + std::to_string(request->httpMajor) + "." + std::to_string(request->httpMinor),
                                          request->queries,
                                          request->headers,
                                          request->cookies,
                                          std::vector<char>());

        if (socketContext != nullptr) {
            if (request != nullptr) {
                if (web::http::ciContains(request->get("connection"), "Upgrade")) {
                    SocketContextUpgradeFactory* socketContextUpgradeFactory =
                        SocketContextUpgradeFactorySelector::instance()->select(*request, *this);

                    if (socketContextUpgradeFactory != nullptr) {
                        name = socketContextUpgradeFactory->name();

                        LOG(DEBUG) << connectionName << " HTTP upgrade: SocketContextUpgradeFactory create success for: " << name;

                        core::socket::stream::SocketContext* socketContextUpgrade =
                            socketContextUpgradeFactory->create(socketContext->getSocketConnection());

                        if (socketContextUpgrade != nullptr) {
                            LOG(DEBUG) << connectionName << " HTTP upgrade: SocketContextUpgrade create success for: " << name;

                            socketContext->switchSocketContext(socketContextUpgrade);
                        } else {
                            LOG(DEBUG) << connectionName << " HTTP upgrade: SocketContextUpgrade create failed for: " << name;

                            set("Connection", "close").status(404);
                        }
                    } else {
                        LOG(DEBUG) << connectionName
                                   << " SocketContextUpgradeFactory create failed for all of: " << request->get("upgrade");

                        set("Connection", "close").status(404);
                    }
                } else {
                    LOG(DEBUG) << connectionName << " HTTP upgrade: No upgrade requested";

                    set("Connection", "close").status(400);
                }
            } else {
                LOG(ERROR) << connectionName << " HTTP upgrade: Request has gone away";

                set("Connection", "close").status(500);
            }
        } else {
            LOG(ERROR) << "HTTP upgrade: Unexpected disconnect";
        }

        status(name);
    }

    void Response::sendFile(const std::string& file, const std::function<void(int)>& callback) {
        if (socketContext != nullptr) {
            std::string absolutFileName = file;

            if (std::filesystem::exists(absolutFileName)) {
                std::error_code ec;
                absolutFileName = std::filesystem::canonical(absolutFileName);

                if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                    core::file::FileReader::open(absolutFileName)->pipe(this, [this, &absolutFileName, &callback](int errnum) {
                        callback(errnum);

                        if (errnum == 0) {
                            set("Content-Type", web::http::MimeTypes::contentType(absolutFileName), false);
                            set("Last-Modified", httputils::file_mod_http_date(absolutFileName), false);
                            if (httpMajor == 1) {
                                if (httpMinor == 1) {
                                    set("Transfer-Encoding", "chunked");
                                } else {
                                    set("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName)));
                                }
                            }
                        } else {
                            status(404);
                        }
                    });
                } else {
                    status(404);
                    errno = EEXIST;
                    callback(errno);
                }
            } else {
                errno = ENOENT;
                callback(errno);
            }
        }
    }

    void Response::end() {
        send("");
    }

    Response& Response::sendHeader() {
        if (socketContext != nullptr) {
            socketContext->responseStarted();

            socketContext->sendToPeer("HTTP/" + std::to_string(httpMajor)
                                                    .append(".")
                                                    .append(std::to_string(httpMinor))
                                                    .append(" ")
                                                    .append(std::to_string(statusCode))
                                                    .append(" ")
                                                    .append(StatusCode::reason(statusCode))
                                                    .append("\r\n"));
            socketContext->sendToPeer("Date: " + httputils::to_http_date() + "\r\n");

            set("X-Powered-By", "snode.c");

            for (const auto& [field, value] : headers) {
                socketContext->sendToPeer(std::string(field).append(": ").append(value).append("\r\n"));
            }

            for (const auto& [cookie, cookieValue] : cookies) {
                const std::string cookieString = std::accumulate(
                    cookieValue.getOptions().begin(),
                    cookieValue.getOptions().end(),
                    cookie + "=" + cookieValue.getValue(),
                    [](const std::string& str, const std::pair<const std::string&, const std::string&> option) -> std::string {
                        return str + "; " + option.first + (!option.second.empty() ? "=" + option.second : "");
                    });
                socketContext->sendToPeer("Set-Cookie: " + cookieString + "\r\n");
            }

            socketContext->sendToPeer("\r\n");
        }

        return *this;
    }

    Response& Response::sendFragment(const char* chunk, std::size_t chunkLen) {
        if (socketContext != nullptr) {
            if (transferEncoding == TransferEncoding::Chunked) {
                socketContext->sendToPeer(to_hex_str(chunkLen).append("\r\n"));
            }

            socketContext->sendToPeer(chunk, chunkLen);
            contentSent += chunkLen;

            if (transferEncoding == TransferEncoding::Chunked) {
                socketContext->sendToPeer("\r\n");
                contentLength += chunkLen;
            }
        }

        return *this;
    }

    Response& Response::sendFragment(const std::string& chunk) {
        return sendFragment(chunk.data(), chunk.size());
    }

    void Response::sendCompleted() {
        if (transferEncoding == TransferEncoding::Chunked) {
            sendFragment(""); // For transfer encoding chunked. Terminate the chunk sequence.

            if (!trailer.empty()) {
                for (auto& [field, value] : trailer) {
                    socketContext->sendToPeer(std::string(field).append(":").append(value).append("\r\n"));
                }

                socketContext->sendToPeer("\r\n");
            }
        }

        if (socketContext != nullptr) {
            socketContext->responseCompleted(contentSent == contentLength || (httpMajor == 1 && httpMinor == 0));
        }
    }

    void Response::onSourceConnect(core::pipe::Source* source) {
        if (socketContext != nullptr) {
            if (socketContext->streamToPeer(source)) {
                sendHeader();

                source->start();
            } else {
                source->stop();
            }
        } else {
            source->stop();
        }
    }

    void Response::onSourceData(const char* chunk, std::size_t chunkLen) {
        sendFragment(chunk, chunkLen);
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
