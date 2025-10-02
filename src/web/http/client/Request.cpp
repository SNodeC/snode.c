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
#include "web/http/TransferEncoding.h"
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

    MasterRequest::MasterRequest(SocketContext* socketContext, const std::string& host)
        : Request(socketContext, host) {
        this->host(hostFieldValue);
    }

    MasterRequest::MasterRequest(MasterRequest&& request) noexcept
        : Request(std::move(request))
        , requestCommands(std::move(request.requestCommands))
        , contentLengthSent(request.contentLengthSent)
        , onResponseReceived(std::move(request.onResponseReceived))
        , onResponseParseError(std::move(request.onResponseParseError)) {
        request.init();
    }

    MasterRequest::~MasterRequest() {
        for (const RequestCommand* requestCommand : requestCommands) {
            delete requestCommand;
        }

        if (!masterRequest.expired() && Sink::isStreaming()) {
            socketContext->streamEof();
        }
    }

    void MasterRequest::init() {
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

    bool
    MasterRequest::send(const char* chunk,
                        std::size_t chunkLen,
                        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                        const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        bool queued = true;

        if (!masterRequest.expired()) {
            const std::shared_ptr<MasterRequest> newRequest = std::make_shared<MasterRequest>(std::move(*this));

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

    bool
    MasterRequest::send(const std::string& chunk,
                        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                        const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        if (!chunk.empty()) {
            set("Content-Type", "text/html; charset=utf-8", false);
        }

        return send(chunk.data(), chunk.size(), onResponseReceived, onResponseParseError);
    }

    bool MasterRequest::upgrade(
        const std::string& url,
        const std::string& protocols,
        const std::function<void(bool)>& onUpgradeInitiate,
        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, bool)>& onResponseReceived,
        const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        if (!masterRequest.expired()) {
            const std::shared_ptr<MasterRequest> newRequest = std::make_shared<MasterRequest>(std::move(*this));

            newRequest->url = url;

            newRequest->requestCommands.push_back(new commands::UpgradeCommand(
                url,
                protocols,
                onUpgradeInitiate,
                [onResponseReceived](const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response) {
                    if (request != nullptr) {
                        const std::string connectionName = request->getSocketContext()->getSocketConnection()->getConnectionName();

                        VLOG(0) << "Request method 3: " << request->method;
                        for (auto& header : request->getHeaders()) {
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

                            onResponseReceived(request, response, !name.empty());
                        });
                    }
                },
                onResponseParseError));

            requestPrepared(newRequest);
        }

        return !masterRequest.expired();
    }

    bool MasterRequest::sendFile(
        const std::string& file,
        const std::function<void(int)>& onStatus,
        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
        const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        bool queued = false;

        if (!masterRequest.expired()) {
            const std::shared_ptr<MasterRequest> newRequest = std::make_shared<MasterRequest>(std::move(*this));

            newRequest->requestCommands.push_back(new commands::SendFileCommand(file, onStatus, onResponseReceived, onResponseParseError));

            requestPrepared(newRequest);

            queued = true;
        }

        return queued;
    }

    MasterRequest& MasterRequest::sendHeader() {
        if (!masterRequest.expired()) {
            requestCommands.push_back(new commands::SendHeaderCommand());
        }

        return *this;
    }

    MasterRequest& MasterRequest::sendFragment(const char* chunk, std::size_t chunkLen) {
        if (!masterRequest.expired()) {
            contentLength += chunkLen;

            requestCommands.push_back(new commands::SendFragmentCommand(chunk, chunkLen));
        }

        return *this;
    }

    MasterRequest& MasterRequest::sendFragment(const std::string& data) {
        return sendFragment(data.data(), data.size());
    }

    bool
    MasterRequest::end(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                       const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        bool queued = true;

        if (!masterRequest.expired()) {
            const std::shared_ptr<MasterRequest> newRequest = std::make_shared<MasterRequest>(std::move(*this));

            newRequest->sendHeader();

            newRequest->requestCommands.push_back(new commands::EndCommand(onResponseReceived, onResponseParseError));

            requestPrepared(newRequest);
        } else {
            queued = false;
        }

        return queued;
    }

    bool MasterRequest::initiate(const std::shared_ptr<MasterRequest>& request) {
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

    bool MasterRequest::executeSendFile(const std::string& file, const std::function<void(int)>& onStatus) {
        bool atomar = true;

        std::string absolutFileName = file;

        if (std::filesystem::exists(absolutFileName)) {
            std::error_code ec;
            absolutFileName = std::filesystem::canonical(absolutFileName);

            if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                core::file::FileReader::open(absolutFileName, [this, &absolutFileName, &onStatus, &atomar](int fd) {
                    onStatus(errno);

                    if (fd >= 0) {
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
                        };
                    } else {
                        executeEnd();
                    }
                })->pipe(this);
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

    bool MasterRequest::executeUpgrade(const std::string& url, const std::string& protocols, const std::function<void(bool)>& onStatus) {
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
                                          trailer,
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

    bool MasterRequest::executeEnd() { // NOLINT
        return true;
    }

    bool MasterRequest::executeSendHeader() {
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

    bool MasterRequest::executeSendFragment(const char* chunk, std::size_t chunkLen) {
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

    void MasterRequest::requestPrepared(const std::shared_ptr<MasterRequest>& request) {
        socketContext->requestPrepared(request);
    }

    void MasterRequest::deliverResponse(const std::shared_ptr<MasterRequest>& request, const std::shared_ptr<Response>& response) {
        onResponseReceived(request, response);
    }

    void MasterRequest::deliverResponseParseError(const std::shared_ptr<MasterRequest>& request, const std::string& message) {
        onResponseParseError(request, message);
    }

    void MasterRequest::requestDelivered() {
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

    void MasterRequest::onSourceConnect(core::pipe::Source* source) {
        if (!masterRequest.expired()) {
            if (socketContext->streamToPeer(source)) {
                source->start();
            }
        } else {
            source->stop();
        }
    }

    void MasterRequest::onSourceData(const char* chunk, std::size_t chunkLen) {
        executeSendFragment(chunk, chunkLen);
    }

    void MasterRequest::onSourceEof() {
        if (!masterRequest.expired()) {
            socketContext->streamEof();

            requestDelivered();
        }
    }

    void MasterRequest::onSourceError(int errnum) {
        errno = errnum;

        if (!masterRequest.expired()) {
            socketContext->streamEof();
            socketContext->close();

            requestDelivered();
        }
    }

} // namespace web::http::client
