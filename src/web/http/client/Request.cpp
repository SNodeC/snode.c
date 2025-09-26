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

#include "web/http/client/Request.h"

#include "core/file/FileReader.h"
#include "core/socket/stream/SocketConnection.h"
#include "web/http/MimeTypes.h"
#include "web/http/client/SocketContext.h"
#include "web/http/client/SocketContextUpgradeFactorySelector.h"

//

#include "commands/EndCommand.h"
#include "commands/SendFileCommand.h"
#include "commands/SendFragmentCommand.h"
#include "commands/SendHeaderCommand.h"
#include "commands/UpgradeCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "web/http/http_utils.h"

#include <cerrno>
#include <filesystem>
#include <sstream>
#include <system_error>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define to_hex_str(int_val) (static_cast<std::ostringstream const&>(std::ostringstream() << std::uppercase << std::hex << int_val)).str()

namespace web::http::client {

    Request::Request(SocketContext* socketContext, const std::string& host)
        : hostFieldValue(host)
        , socketContext(socketContext) {
        this->host(hostFieldValue);
    }

    Request::Request(Request&& request) noexcept
        : hostFieldValue(request.hostFieldValue) // NOLINT
        , method(std::move(request.method))
        , url(std::move(request.url))
        , httpMajor(request.httpMajor)
        , httpMinor(request.httpMinor)
        , queries(std::move(request.queries))
        , headers(std::move(request.headers))
        , cookies(std::move(request.cookies))
        , trailer(std::move(request.trailer))
        , requestCommands(std::move(request.requestCommands))
        , transferEncoding(request.transferEncoding)
        , contentLength(request.contentLength)
        , contentLengthSent(request.contentLengthSent)
        , connectionState(request.connectionState)
        , onResponseReceived(std::move(request.onResponseReceived))
        , onResponseParseError(std::move(request.onResponseParseError))
        , masterRequest(request.masterRequest) // NOLINT
        , socketContext(request.socketContext) {
        request.init();
    }

    Request::~Request() {
        for (const RequestCommand* requestCommand : requestCommands) {
            delete requestCommand;
        }

        if (!masterRequest.expired() && Sink::isStreaming()) {
            socketContext->streamEof();
        }
    }

    void Request::setMasterRequest(const std::shared_ptr<Request>& masterRequest) {
        this->masterRequest = masterRequest;
    }

    void Request::init() {
        method = "GET";
        url = "/";
        httpMajor = 1;
        httpMinor = 1;
        queries.clear();
        headers.clear();
        cookies.clear();
        trailer.clear();
        requestCommands.clear();
        transferEncoding = TransferEncoding::HTTP10;
        contentLength = 0;
        contentLengthSent = 0;
        connectionState = ConnectionState::Default;

        this->host(hostFieldValue);
        set("X-Powered-By", "snode.c");
    }

    Request& Request::host(const std::string& hostFieldValue) {
        set("Host", hostFieldValue);

        return *this;
    }

    Request& Request::append(const std::string& field, const std::string& value) {
        const std::map<std::string, std::string>::iterator it = headers.find(field);

        if (it != headers.end()) {
            set(field, it->second.append(", ").append(value));
        } else {
            set(field, value);
        }

        return *this;
    }

    Request& Request::set(const std::string& field, const std::string& value, bool overwrite) {
        if (!value.empty()) {
            if (overwrite) {
                headers.insert_or_assign(field, value);
            } else {
                headers.insert({field, value});
            }

            if (web::http::ciEquals(field, "Connection")) {
                if (web::http::ciContains(headers[field], "close")) {
                    connectionState = ConnectionState::Close;
                } else if (web::http::ciContains(headers[field], "keep-alive")) {
                    connectionState = ConnectionState::Keep;
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

    Request& Request::set(const std::map<std::string, std::string>& headers, bool overwrite) {
        for (const auto& [field, value] : headers) {
            set(field, value, overwrite);
        }

        return *this;
    }

    Request& Request::type(const std::string& type) {
        headers.insert({"Content-Type", type});

        return *this;
    }

    Request& Request::cookie(const std::string& name, const std::string& value) {
        cookies.insert({name, value});

        return *this;
    }

    Request& Request::cookie(const std::map<std::string, std::string>& cookies) {
        for (const auto& [name, value] : cookies) {
            cookie(name, value);
        }

        return *this;
    }

    Request& Request::query(const std::string& key, const std::string& value) {
        queries.insert({key, value});

        return *this;
    }

    Request& Request::setTrailer(const std::string& field, const std::string& value, bool overwrite) {
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

    bool Request::send(const char* chunk,
                       std::size_t chunkLen,
                       const std::function<void(const std::shared_ptr<Response>&)>& onResponseReceived,
                       const std::function<void(const std::string&)>& onResponseParseError) {
        bool queued = true;

        if (!masterRequest.expired()) {
            std::shared_ptr<Request> newRequest = std::make_shared<Request>(std::move(*this));

            if (chunkLen > 0) {
                newRequest->set("Content-Type", "application/octet-stream", false);
            }

            newRequest->sendHeader();
            newRequest->sendFragment(chunk, chunkLen);

            newRequest->requestCommands.push_back(new commands::EndCommand(onResponseReceived, onResponseParseError));

            newRequest->requestPrepared(newRequest);

        } else {
            queued = false;
        }

        return queued;
    }

    bool Request::send(const std::string& chunk,
                       const std::function<void(const std::shared_ptr<Response>&)>& onResponseReceived,
                       const std::function<void(const std::string&)>& onResponseParseError) {
        if (!chunk.empty()) {
            set("Content-Type", "text/html; charset=utf-8", false);
        }

        return send(chunk.data(), chunk.size(), onResponseReceived, onResponseParseError);
    }

    bool Request::upgrade(const std::string& url,
                          const std::string& protocols,
                          const std::function<void(bool)>& onUpgradeInitiate,
                          const std::function<void(const std::shared_ptr<Response>&, bool)>& onResponseReceived,
                          const std::function<void(const std::string&)>& onResponseParseError) {
        if (!masterRequest.expired()) {
            const std::shared_ptr<Request> newRequest = std::make_shared<Request>(std::move(*this));

            newRequest->url = url;

            newRequest->requestCommands.push_back(new commands::UpgradeCommand(
                url,
                protocols,
                onUpgradeInitiate,
                [originRequest = std::weak_ptr(newRequest), onResponseReceived](const std::shared_ptr<Response>& response) {
                    const std::shared_ptr<Request> request = originRequest.lock();

                    if (request != nullptr) {
                        const std::string connectionName = originRequest.lock()->socketContext->getSocketConnection()->getConnectionName();

                        VLOG(0) << "Request method 3: " << request->method;
                        for (auto& header : request->headers) {
                            VLOG(0) << "  Header: " << header.first << " : " << header.second;
                        }

                        LOG(DEBUG) << connectionName << " HTTP upgrade: Response to upgrade request: " << request->method << " "
                                   << request->url << " "
                                   << "HTTP/" << request->httpMajor << "." << request->httpMinor << "\n"
                                   << httputils::toString(response->httpVersion,
                                                          response->statusCode,
                                                          response->reason,
                                                          response->headers,
                                                          response->cookies,
                                                          response->body);

                        request->upgrade(response, [request, response, connectionName, &onResponseReceived](const std::string& name) {
                            LOG(DEBUG) << connectionName << " HTTP upgrade: bootstrap " << (!name.empty() ? "success" : "failed");
                            LOG(DEBUG) << "      Protocol selected: " << name;
                            LOG(DEBUG) << "              requested: " << request->header("upgrade");
                            LOG(DEBUG) << "  Subprotocol  selected: " << response->get("Sec-WebSocket-Protocol");
                            LOG(DEBUG) << "              requested: " << request->header("Sec-WebSocket-Protocol");

                            onResponseReceived(response, !name.empty());
                        });
                    }
                },
                onResponseParseError));

            requestPrepared(newRequest);
        }

        return !masterRequest.expired();
    }

    void Request::upgrade(const std::shared_ptr<Response>& response, const std::function<void(const std::string&)>& status) {
        const std::string connectionName = socketContext->getSocketConnection()->getConnectionName();

        std::string name;

        if (!masterRequest.expired()) {
            if (response != nullptr) {
                if (web::http::ciContains(response->get("connection"), "Upgrade")) {
                    SocketContextUpgradeFactory* socketContextUpgradeFactory =
                        SocketContextUpgradeFactorySelector::instance()->select(*this, *response);

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

                            socketContext->close();
                        }
                    } else {
                        LOG(DEBUG) << connectionName
                                   << " HTTP upgrade: SocketContextUpgradeFactory not supported by server: " << header("upgrade");

                        socketContext->close();
                    }
                } else {
                    LOG(DEBUG) << connectionName << " HTTP upgrade: No upgrade requested";

                    socketContext->close();
                }
            } else {
                LOG(ERROR) << connectionName << " HTTP upgrade: Response has gone away";

                socketContext->close();
            }
        } else {
            LOG(ERROR) << connectionName << " HTTP upgrade: Unexpected disconnect";
        }

        status(name);
    }

    bool Request::sendFile(const std::string& file,
                           const std::function<void(int)>& onStatus,
                           const std::function<void(const std::shared_ptr<Response>&)>& onResponseReceived,
                           const std::function<void(const std::string&)>& onResponseParseError) {
        bool queued = false;

        if (!masterRequest.expired()) {
            const std::shared_ptr<Request> newRequest = std::make_shared<Request>(std::move(*this));

            newRequest->requestCommands.push_back(new commands::SendFileCommand(file, onStatus, onResponseReceived, onResponseParseError));

            requestPrepared(newRequest);

            queued = true;
        }

        return queued;
    }

    Request& Request::sendHeader() {
        if (!masterRequest.expired()) {
            requestCommands.push_back(new commands::SendHeaderCommand());
        }

        return *this;
    }

    Request& Request::sendFragment(const char* chunk, std::size_t chunkLen) {
        if (!masterRequest.expired()) {
            contentLength += chunkLen;

            requestCommands.push_back(new commands::SendFragmentCommand(chunk, chunkLen));
        }

        return *this;
    }

    Request& Request::sendFragment(const std::string& data) {
        return sendFragment(data.data(), data.size());
    }

    bool Request::end(const std::function<void(const std::shared_ptr<Response>&)>& onResponseReceived,
                      const std::function<void(const std::string&)>& onResponseParseError) {
        bool queued = true;

        if (!masterRequest.expired()) {
            const std::shared_ptr<Request> newRequest = std::make_shared<Request>(std::move(*this));

            newRequest->sendHeader();

            newRequest->requestCommands.push_back(new commands::EndCommand(onResponseReceived, onResponseParseError));

            requestPrepared(newRequest);
        } else {
            queued = false;
        }

        return queued;
    }

    bool Request::initiate(std::shared_ptr<Request> request) {
        bool error = false;
        bool atomar = true;

        for (RequestCommand* requestCommand : requestCommands) {
            if (!error) {
                this->onResponseParseError = requestCommand->onResponseParseError;
                this->onResponseReceived = requestCommand->onResponseReceived;

                atomar = requestCommand->execute(request);
                error = requestCommand->getError();
            }

            delete requestCommand;
        }
        requestCommands.clear();

        if (atomar && !error) {
            requestDelivered();
        }

        return !error;
    }

    bool Request::executeSendFile(const std::string& file, const std::function<void(int)>& onStatus) {
        bool atomar = true;

        std::string absolutFileName = file;

        if (std::filesystem::exists(absolutFileName)) {
            std::error_code ec;
            absolutFileName = std::filesystem::canonical(absolutFileName);

            if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                core::file::FileReader::open(absolutFileName)->pipe(this, [this, &atomar, &absolutFileName, &onStatus](int errnum) {
                    errno = errnum;
                    onStatus(errnum);
                    if (errnum == 0) {
                        if (httpMajor == 1) {
                            atomar = false;

                            set("Content-Type", web::http::MimeTypes::contentType(absolutFileName), false);
                            set("Last-Modified", httputils::file_mod_http_date(absolutFileName), false);
                            if (httpMinor == 1 && contentLength == 0) {
                                set("Transfer-Encoding", "chunked");
                            } else {
                                set("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName) + contentLength));
                            }

                            executeSendHeader();
                        }
                    }
                });
            } else {
                errno = EINVAL;
                onStatus(errno);
            }
        } else {
            errno = ENOENT;
            onStatus(errno);
        }

        return atomar;
    }

    bool Request::executeUpgrade(const std::string& url, const std::string& protocols, const std::function<void(bool)>& onStatus) {
        const std::string connectionName = this->getSocketContext()->getSocketConnection()->getConnectionName();
        this->url = url;

        VLOG(0) << "Request method 2: " << method;
        for (auto& header : headers) {
            VLOG(0) << "  Header: " << header.first << " : " << header.second;
        }

        set("Connection", "Upgrade", true);
        set("Upgrade", protocols, true);

        web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory =
            web::http::client::SocketContextUpgradeFactorySelector::instance()->select(protocols, *this);

        if (socketContextUpgradeFactory != nullptr) {
            LOG(DEBUG) << connectionName << " HTTP: "
                       << "SocketContextUpgradeFactory create success: " << socketContextUpgradeFactory->name();
            LOG(DEBUG) << connectionName << " HTTP: Initiating upgrade: " << method << " " << url
                       << " HTTP/" + std::to_string(httpMajor) + "." + std::to_string(httpMinor);

        } else {
            LOG(DEBUG) << connectionName << " HTTP: "
                       << "SocketContextUpgradeFactory create failed: " << protocols;
            LOG(DEBUG) << connectionName << " HTTP: Not initiating upgrade " << method << " " << url
                       << " HTTP/" + std::to_string(httpMajor) + "." + std::to_string(httpMinor);
        }

        LOG(DEBUG) << connectionName << " HTTP: Upgrade request:\n"
                   << httputils::toString(method,
                                          url,
                                          "HTTP/" + std::to_string(httpMajor) + "." + std::to_string(httpMinor),
                                          queries,
                                          headers,
                                          cookies,
                                          std::vector<char>());

        onStatus(socketContextUpgradeFactory != nullptr);

        if (socketContextUpgradeFactory != nullptr) {
            executeSendHeader();

            socketContextUpgradeFactory->checkRefCount();
        } else {
            socketContext->close();
        }

        return true;
    }

    bool Request::executeEnd() { // NOLINT
        return true;
    }

    bool Request::executeSendHeader() {
        const std::string httpVersion = "HTTP/" + std::to_string(httpMajor) + "." + std::to_string(httpMinor);

        std::string queryString;
        if (!queries.empty()) {
            queryString += "?";
            for (auto& [key, value] : queries) {
                queryString += httputils::url_encode(key) + "=" + httputils::url_encode(value) + "&";
            }
            queryString.pop_back();
        }

        socketContext->sendToPeer(method + " " + url + queryString + " " + httpVersion + "\r\n");
        socketContext->sendToPeer("Date: " + httputils::to_http_date() + "\r\n");

        if (!headers.contains("Transfer-Encoding") && contentLength > 0) {
            set("Content-Length", std::to_string(contentLength));
        }

        for (const auto& [field, value] : headers) {
            socketContext->sendToPeer(std::string(field).append(":").append(value).append("\r\n"));
        }

        for (const auto& [name, value] : cookies) {
            socketContext->sendToPeer(std::string("Cookie:").append(name).append("=").append(value).append("\r\n"));
        }

        socketContext->sendToPeer("\r\n");

        return true;
    }

    bool Request::executeSendFragment(const char* chunk, std::size_t chunkLen) {
        if (transferEncoding == TransferEncoding::Chunked) {
            socketContext->sendToPeer(to_hex_str(chunkLen).append("\r\n"));
        }

        socketContext->sendToPeer(chunk, chunkLen);
        contentLengthSent += chunkLen;

        if (transferEncoding == TransferEncoding::Chunked) {
            socketContext->sendToPeer("\r\n");
            contentLength += chunkLen;
        }

        return true;
    }

    void Request::requestPrepared(std::shared_ptr<Request> request) {
        socketContext->requestPrepared(request);
    }

    void Request::deliverResponse(const std::shared_ptr<Response>& response) {
        onResponseReceived(response);
    }

    void Request::deliverResponseParseError(const std::string& message) {
        onResponseParseError(message);
    }

    void Request::requestDelivered() {
        if (!masterRequest.expired()) {
            if (transferEncoding == TransferEncoding::Chunked) {
                executeSendFragment("", 0); // For transfer encoding chunked. Terminate the chunk sequence.

                if (!trailer.empty()) {
                    for (auto& [field, value] : trailer) {
                        socketContext->sendToPeer(std::string(field).append(":").append(value).append("\r\n"));
                    }
                    socketContext->sendToPeer("\r\n");
                }
            }

            socketContext->requestDelivered(contentLengthSent == contentLength);
        }
    }

    void Request::onSourceConnect(core::pipe::Source* source) {
        if (!masterRequest.expired()) {
            if (socketContext->streamToPeer(source)) {
                source->start();
            }
        } else {
            source->stop();
        }
    }

    void Request::onSourceData(const char* chunk, std::size_t chunkLen) {
        executeSendFragment(chunk, chunkLen);
    }

    void Request::onSourceEof() {
        if (!masterRequest.expired()) {
            socketContext->streamEof();

            requestDelivered();
        }
    }

    void Request::onSourceError(int errnum) {
        errno = errnum;

        if (!masterRequest.expired()) {
            socketContext->streamEof();
            socketContext->close();

            requestDelivered();
        }
    }

    std::string Request::header(const std::string& field) {
        return headers.contains(field) ? headers[field] : "";
    }

    const web::http::CiStringMap<std::string>& Request::getQueries() const {
        return queries;
    }

    const web::http::CiStringMap<std::string>& Request::getHeaders() const {
        return headers;
    }

    const web::http::CiStringMap<std::string>& Request::getCookies() const {
        return cookies;
    }

    SocketContext* Request::getSocketContext() const {
        return socketContext;
    }

} // namespace web::http::client
