/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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
#include "core/socket/stream/QueueResult.h"
#include "core/socket/stream/SocketConnection.h"
#include "web/http/MimeTypes.h"
#include "web/http/StatusCodes.h"
#include "web/http/server/SemanticLog.h"
#include "web/http/server/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/SemanticLogger.h"
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
        disconnect();
    }

    void Response::disconnect() {
        if (isConnected() && Sink::isStreaming()) {
            socketContext->streamEof();
        }

        stop();
        socketContext = nullptr;
    }

    bool Response::isConnected() const {
        return socketContext != nullptr;
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
        sourceFailed = false;
        sourceConnectAccepted = false;
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
        if (isConnected()) {
            if (chunkLen > 0) {
                set("Content-Type", "application/octet-stream", false);
            }
            set("Content-Length", std::to_string(chunkLen));

            sendHeader();
            sendFragment(chunk, chunkLen);
            sendCompleted();
        }
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
        if (isConnected()) {
            auto log = semantic::httpServerLog(*socketContext->getSocketConnection());

            std::string socketContextUpgradeName;

            if (request != nullptr) {
                if (log.enabled(logger::LogLevel::Debug)) {
                    const std::string prefix = "Initiating upgrade: " + request->method + " " + request->url + " HTTP/" +
                                               std::to_string(httpMajor) + "." + std::to_string(httpMinor) + "\n";
                    const auto formatted = httputils::toStringPresentation(request->method,
                                                                           request->url,
                                                                           "HTTP/" + std::to_string(request->httpMajor) + "." +
                                                                               std::to_string(request->httpMinor),
                                                                           request->queries,
                                                                           request->headers,
                                                                           {},
                                                                           request->cookies,
                                                                           std::vector<char>());
                    log.emit(logger::LogLevel::Debug,
                             logger::PresentedMessage{.plain = prefix + formatted.plain, .terminal = prefix + formatted.terminal});
                }
                if (web::http::ciContains(request->get("connection"), "Upgrade")) {
                    SocketContextUpgradeFactory* socketContextUpgradeFactory =
                        SocketContextUpgradeFactorySelector::instance()->select(*request, *this);

                    if (socketContextUpgradeFactory != nullptr) {
                        socketContextUpgradeName = socketContextUpgradeFactory->name();

                        log.debug() << "SocketContextUpgradeFactory create success for: " << socketContextUpgradeName;

                        core::socket::stream::SocketContext* socketContextUpgrade =
                            socketContextUpgradeFactory->create(socketContext->getSocketConnection());

                        if (socketContextUpgrade != nullptr) {
                            log.debug() << "SocketContextUpgrade create success for: " << socketContextUpgradeName;

                            if (log.enabled(logger::LogLevel::Debug)) {
                                const std::string prefix = "Response to upgrade request: " + request->method + " " + request->url +
                                                           " HTTP/" + std::to_string(request->httpMajor) + "." +
                                                           std::to_string(request->httpMinor) + "\n";
                                const auto formatted =
                                    httputils::toStringPresentation("HTTP/" + std::to_string(httpMajor) + "." + std::to_string(httpMinor),
                                                                    std::to_string(statusCode),
                                                                    StatusCode::reason(statusCode),
                                                                    headers,
                                                                    cookies,
                                                                    {});
                                log.emit(
                                    logger::LogLevel::Debug,
                                    logger::PresentedMessage{.plain = prefix + formatted.plain, .terminal = prefix + formatted.terminal});
                            }

                            socketContext->getSocketConnection()->setSocketContext(socketContextUpgrade);
                        } else {
                            log.debug() << "SocketContextUpgrade create failed for: " << socketContextUpgradeName;

                            set("Connection", "close").status(404);
                        }
                    } else {
                        log.debug() << "SocketContextUpgradeFactory create failed for all of: " << request->get("upgrade");

                        set("Connection", "close").status(404);
                    }
                } else {
                    log.debug() << "No upgrade requested";

                    set("Connection", "close").status(400);
                }
            } else {
                log.error() << "Upgrade request has gone away";

                set("Connection", "close").status(500);

                log.debug() << "Upgrade bootstrap failed";
                log.debug() << "      Protocol selected: ";
                log.debug() << "  Subprotocol  selected: " << header("Sec-WebSocket-Protocol");

                status({});
                return;
            }

            log.debug() << "Upgrade bootstrap " << (!socketContextUpgradeName.empty() ? "success" : "failed");
            log.debug() << "      Protocol selected: " << socketContextUpgradeName;
            log.debug() << "              requested: " << request->get("upgrade");
            log.debug() << "  Subprotocol  selected: " << header("Sec-WebSocket-Protocol");
            log.debug() << "              requested: " << request->get("Sec-WebSocket-Protocol");

            status(socketContextUpgradeName);
        } else {
            semantic::httpServerLog().error() << "Unexpected disconnect during upgrade";
        }
    }

    void Response::sendFile(const std::string& file, const std::function<void(int)>& onStatus) {
        if (isConnected()) {
            std::string absolutFileName = file;

            if (std::filesystem::exists(absolutFileName)) {
                std::error_code ec;
                absolutFileName = std::filesystem::canonical(absolutFileName);

                if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                    core::file::FileReader* fileReader =
                        core::file::FileReader::open(absolutFileName, [this, &absolutFileName, &onStatus](int fd) {
                            if (fd >= 0) {
                                status(200);
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
                            onStatus(errno);
                        });

                    if (fileReader != nullptr && !pipe(fileReader)) {
                        fileReader->stop();
                    }
                } else {
                    status(404);
                    errno = EEXIST;
                    onStatus(errno);
                }
            } else {
                status(404);
                errno = ENOENT;
                onStatus(errno);
            }
        }
    }

    bool Response::pipe(core::pipe::Source* source) {
        sourceConnectAccepted = false;

        return source != nullptr && isConnected() && !Sink::isStreaming() && source->pipe(this) && sourceConnectAccepted;
    }

    void Response::end() {
        send("");
    }

    Response& Response::sendHeader() {
        if (isConnected()) {
            socketContext->responseStarted(*this);

            socketContext->sendToPeer("HTTP/" + std::to_string(httpMajor)
                                                    .append(".")
                                                    .append(std::to_string(httpMinor))
                                                    .append(" ")
                                                    .append(std::to_string(statusCode))
                                                    .append(" ")
                                                    .append(StatusCode::reason(statusCode))
                                                    .append("\r\n"));
            socketContext->sendToPeer("Date: " + httputils::to_http_date() + "\r\n");

            set("X-Powered-By", "SNode.C");

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
        // RFC 9110 / Express semantics: HEAD responses must not include a message body.
        if (requestMethod == "HEAD") {
            return *this;
        }
        if (isConnected()) {
            if (transferEncoding == TransferEncoding::Chunked) {
                socketContext->sendToPeer(to_hex_str(chunkLen).append("\r\n"));
            }

            socketContext->sendToPeer(chunk, chunkLen);
            contentSent += chunkLen;

            if (transferEncoding == TransferEncoding::Chunked || web::http::ciContains(headers["Content-Type"], "text/event-stream")) {
                socketContext->sendToPeer("\r\n");
                contentLength += chunkLen;
            }
        }

        return *this;
    }

    Response& Response::sendFragment(const std::string& chunk) {
        return sendFragment(chunk.data(), chunk.size());
    }

    void Response::sendCompleted(bool boundedStream) {
        if (isConnected()) {
            if (!sourceFailed && transferEncoding == TransferEncoding::Chunked) {
                if (boundedStream) {
                    std::string completion = "0\r\n";

                    if (!trailer.empty()) {
                        for (const auto& [field, value] : trailer) {
                            completion.append(field).append(": ").append(value).append("\r\n");
                        }
                    }

                    completion.append("\r\n");

                    const core::socket::stream::QueueResult queueResult = socketContext->trySendToPeer(completion);
                    if (queueResult != core::socket::stream::QueueResult::Queued) {
                        sourceFailed = true;
                        socketContext->close();
                    }
                } else {
                    sendFragment(""); // For transfer encoding chunked. Terminate the chunk sequence.

                    if (!trailer.empty()) {
                        for (auto& [field, value] : trailer) {
                            socketContext->sendToPeer(std::string(field).append(":").append(value).append("\r\n"));
                        }

                        socketContext->sendToPeer("\r\n");
                    }
                }
            }

            const bool isHead = (requestMethod == "HEAD");
            if (isHead) {
                // Pretend we sent the full body length; this prevents keep-alive logic from treating HEAD as incomplete.
                contentSent = contentLength;
            }
            socketContext->responseCompleted(
                *this, !sourceFailed && (isHead || contentSent == contentLength || (httpMajor == 1 && httpMinor == 0)));
        }
    }

    core::socket::stream::QueueResult Response::trySendHeader() {
        using core::socket::stream::QueueResult;

        if (!isConnected()) {
            return QueueResult::Closed;
        }

        socketContext->responseStarted(*this);

        std::string responseHeader = "HTTP/" + std::to_string(httpMajor)
                                                   .append(".")
                                                   .append(std::to_string(httpMinor))
                                                   .append(" ")
                                                   .append(std::to_string(statusCode))
                                                   .append(" ")
                                                   .append(StatusCode::reason(statusCode))
                                                   .append("\r\n");
        responseHeader.append("Date: ").append(httputils::to_http_date()).append("\r\n");

        set("X-Powered-By", "SNode.C");

        for (const auto& [field, value] : headers) {
            responseHeader.append(field).append(": ").append(value).append("\r\n");
        }

        for (const auto& [cookie, cookieValue] : cookies) {
            const std::string cookieString =
                std::accumulate(cookieValue.getOptions().begin(),
                                cookieValue.getOptions().end(),
                                cookie + "=" + cookieValue.getValue(),
                                [](const std::string& str, const std::pair<const std::string&, const std::string&> option) -> std::string {
                                    return str + "; " + option.first + (!option.second.empty() ? "=" + option.second : "");
                                });
            responseHeader.append("Set-Cookie: ").append(cookieString).append("\r\n");
        }

        responseHeader.append("\r\n");

        return socketContext->trySendToPeer(responseHeader);
    }

    core::socket::stream::QueueResult Response::trySendFragment(const char* chunk, std::size_t chunkLen) {
        using core::socket::stream::QueueResult;

        if (requestMethod == "HEAD") {
            return QueueResult::Queued;
        }

        if (!isConnected()) {
            return QueueResult::Closed;
        }

        const bool serverSentEvent = web::http::ciContains(headers["Content-Type"], "text/event-stream");
        std::string fragment;

        if (transferEncoding == TransferEncoding::Chunked) {
            fragment = to_hex_str(chunkLen).append("\r\n");
        }

        if (chunkLen != 0) {
            fragment.append(chunk, chunkLen);
        }

        if (transferEncoding == TransferEncoding::Chunked || serverSentEvent) {
            fragment.append("\r\n");
        }

        const QueueResult queueResult = socketContext->trySendToPeer(fragment);
        if (queueResult == QueueResult::Queued) {
            contentSent += chunkLen;

            if (transferEncoding == TransferEncoding::Chunked || serverSentEvent) {
                contentLength += chunkLen;
            }
        }

        return queueResult;
    }

    void Response::onSourceQueueError(core::socket::stream::QueueResult queueResult) {
        using core::socket::stream::QueueResult;

        switch (queueResult) {
            case QueueResult::Queued:
                return;
            case QueueResult::WouldExceedLimit:
                errno = ENOBUFS;
                break;
            case QueueResult::Closed:
                errno = EPIPE;
                break;
            case QueueResult::ShutdownInProgress:
#ifdef ESHUTDOWN
                errno = ESHUTDOWN;
#else
                errno = EPIPE;
#endif
                break;
        }

        sourceFailed = true;

        if (isConnected()) {
            socketContext->streamEof();
        }

        stop();

        if (isConnected()) {
            socketContext->close();
        }

        sendCompleted();
    }

    void Response::onSourceConnect(core::pipe::Source* source) {
        if (isConnected()) {
            if (socketContext->streamToPeer(source)) {
                if (requestMethod != "HEAD" && httpMajor == 1 && httpMinor == 1 && !headers.contains("Content-Length") &&
                    !headers.contains("Transfer-Encoding")) {
                    set("Transfer-Encoding", "chunked");
                }

                const core::socket::stream::QueueResult queueResult = trySendHeader();
                if (queueResult == core::socket::stream::QueueResult::Queued) {
                    sourceConnectAccepted = true;
                    source->start();
                } else {
                    onSourceQueueError(queueResult);
                }
            } else {
                stop();
            }
        } else {
            stop();
        }
    }

    void Response::onSourceData(const char* chunk, std::size_t chunkLen) {
        if (requestMethod != "HEAD" && transferEncoding == TransferEncoding::Identity &&
            (contentSent > contentLength || chunkLen > contentLength - contentSent)) {
            onSourceError(EMSGSIZE);
            return;
        }

        onSourceQueueError(trySendFragment(chunk, chunkLen));
    }

    void Response::onSourceEof() {
        if (isConnected()) {
            socketContext->streamEof();
        }

        stop();
        sendCompleted(true);
    }

    void Response::onSourceError(int errnum) {
        errno = errnum;
        sourceFailed = true;

        if (isConnected()) {
            socketContext->streamEof();
        }

        stop();

        if (isConnected()) {
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
