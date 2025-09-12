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
        for (const RequestCommand* requestCommand : requestCommands) {
            delete requestCommand;
        }
        requestCommands.clear();
        transferEncoding = TransferEncoding::HTTP10;
        contentLength = 0;
        contentLengthSent = 0;
        connectionState = ConnectionState::Default;
        onResponseReceived = nullptr;
        onResponseParseError = nullptr;

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

    void Request::responseParseError(const std::shared_ptr<Request>& request, const std::string& message) {
        LOG(WARNING) << request->getSocketContext()->getSocketConnection()->getConnectionName()
                     << " HTTP: Response parse error: " << request->method << " " << request->url << " "
                     << "HTTP/" << request->httpMajor << "." << request->httpMinor << ": " << message;
    }

    bool Request::send(const char* chunk,
                       std::size_t chunkLen,
                       const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                       const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        bool queued = true;

        if (!masterRequest.expired()) {
            this->onResponseReceived = onResponseReceived;
            this->onResponseParseError = onResponseParseError;

            if (chunkLen > 0) {
                set("Content-Type", "application/octet-stream", false);
            }

            sendHeader();
            sendFragment(chunk, chunkLen);

            requestCommands.push_back(new commands::EndCommand());

            requestPrepared();
        } else {
            queued = false;
        }

        return queued;
    }

    bool Request::send(const std::string& chunk,
                       const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                       const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        if (!chunk.empty()) {
            set("Content-Type", "text/html; charset=utf-8", false);
        }

        return send(chunk.data(), chunk.size(), onResponseReceived, onResponseParseError);
    }

    bool
    Request::upgrade(const std::string& url,
                     const std::string& protocols,
                     const std::function<void(const std::shared_ptr<Request>&, bool)>& onUpgradeInitiate,
                     const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, bool)>& onResponseReceived,
                     const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        if (!masterRequest.expired()) {
            this->onResponseParseError = onResponseParseError;

            this->onResponseReceived = [onResponseReceived](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                const std::string connectionName = req->getSocketContext()->getSocketConnection()->getConnectionName();
                LOG(DEBUG) << connectionName << "  HTTP: Response to upgrade request: " << req->method << " " << req->url << " " << "HTTP/"
                           << req->httpMajor << "." << req->httpMinor << "\n"
                           << httputils::toString(res->httpVersion, res->statusCode, res->reason, res->headers, res->cookies, res->body);

                req->upgrade(res, [req, res, connectionName, &onResponseReceived](const std::string& name) {
                    onResponseReceived(req, res, !name.empty());

                    LOG(DEBUG) << connectionName << " HTTP: Upgrade bootstrap " << (!name.empty() ? "success" : "failed");
                    LOG(DEBUG) << "      Protocol selected: " << name;
                    LOG(DEBUG) << "              requested: " << req->header("upgrade");
                    LOG(DEBUG) << "  Subprotocol  selected: " << res->headers["Sec-WebSocket-Protocol"];
                    LOG(DEBUG) << "              requested: " << req->getHeaders().at("Sec-WebSocket-Protocol");
                });
            };

            requestCommands.push_back(new commands::UpgradeCommand(url, protocols, onUpgradeInitiate));

            requestPrepared();
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
                    LOG(DEBUG) << connectionName << " HTTP upgrade: Not any protocol supported by server: " << header("upgrade");

                    socketContext->close();
                }
            } else {
                LOG(ERROR) << socketContext->getSocketConnection()->getConnectionName() << " HTTP upgrade: Response has gone away";

                socketContext->close();
            }
        } else {
            LOG(ERROR) << socketContext->getSocketConnection()->getConnectionName() << " HTTP upgrade: SocketContext has gone away";
        }

        status(name);
    }

    bool Request::sendFile(const std::string& file,
                           const std::function<void(int)>& onStatus,
                           const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                           const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        bool queued = false;

        if (!masterRequest.expired()) {
            this->onResponseReceived = onResponseReceived;
            this->onResponseParseError = onResponseParseError;

            requestCommands.push_back(new commands::SendFileCommand(file, onStatus));

            requestPrepared();

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

    bool Request::end(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                      const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        bool queued = true;

        if (!masterRequest.expired()) {
            this->onResponseReceived = onResponseReceived;
            this->onResponseParseError = onResponseParseError;

            sendHeader();

            requestCommands.push_back(new commands::EndCommand());

            requestPrepared();
        } else {
            queued = false;
        }

        return queued;
    }

    bool Request::initiate(const std::shared_ptr<Request>& request) {
        bool error = false;
        bool atomar = true;

        for (RequestCommand* requestCommand : requestCommands) {
            if (!error) {
                const bool atomarCommand = requestCommand->execute(request);
                if (atomar) {
                    atomar = atomarCommand;
                }

                error = requestCommand->getError();
            }

            delete requestCommand;
        }
        requestCommands.clear();

        if (atomar && (!error || contentLengthSent != 0)) {
            requestDelivered();
        }

        return !error || contentLengthSent != 0;
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

        set("Connection", "Upgrade", true);
        set("Upgrade", protocols, true);

        web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory =
            web::http::client::SocketContextUpgradeFactorySelector::instance()->select(protocols, *this);

        if (socketContextUpgradeFactory != nullptr) {
            LOG(DEBUG) << connectionName << " HTTP: "
                       << "SocketContextUpgradeFactory create success: " << socketContextUpgradeFactory->name();

            LOG(DEBUG) << connectionName << " HTTP: Initiating upgrade: " << method << " " << url
                       << " HTTP/" + std::to_string(httpMajor) + "." + std::to_string(httpMinor) << "\n"
                       << httputils::toString(method,
                                              url,
                                              "HTTP/" + std::to_string(httpMajor) + "." + std::to_string(httpMinor),
                                              queries,
                                              headers,
                                              cookies,
                                              std::vector<char>());

            onStatus(true);

            executeSendHeader();

            socketContextUpgradeFactory->checkRefCount();
        } else {
            LOG(DEBUG) << connectionName << " HTTP: "
                       << "SocketContextUpgradeFactory create failed: " << protocols;
            LOG(DEBUG) << connectionName << " HTTP: Initiating upgrade success";
            LOG(DEBUG) << "  Request: GET " << this->url << " HTTP/1.1";
            LOG(DEBUG) << "  Upgrade:" << header("upgrade");
            LOG(TRACE) << "  Headers:";
            for (const auto& [field, value] : this->getHeaders()) {
                LOG(TRACE) << "    " << field + " = " + value;
            }

            onStatus(false);

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

    void Request::requestPrepared() {
        socketContext->requestPrepared(std::move(*this));
        init();
    }

    void Request::deliverResponse(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response) {
        onResponseReceived(request, response);
    }

    void Request::deliverResponseParseError(const std::shared_ptr<Request>& request, const std::string& message) {
        onResponseParseError(request, message);
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

            socketContext->requestDelivered(std::move(*this), contentLengthSent == contentLength);
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
