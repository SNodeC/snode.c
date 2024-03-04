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

#include "web/http/client/Request.h"

#include "SocketContext.h"
#include "core/file/FileReader.h"
#include "web/http/MimeTypes.h"
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

    Request::Request(web::http::client::SocketContext* clientContext, const std::string& host)
        : socketContext(clientContext) {
        this->host(host);
    }

    Request::Request(Request&& request) noexcept
        : onResponseReceived(std::move(request.onResponseReceived))
        , onResponseParseError(std::move(request.onResponseParseError))
        , method(std::move(request.method))
        , url(std::move(request.url))
        , httpMajor(request.httpMajor)
        , httpMinor(request.httpMinor)
        , queries(std::move(request.queries))
        , headers(std::move(request.headers))
        , cookies(std::move(request.cookies))
        , contentSent(request.contentSent)
        , contentLength(request.contentLength)
        , requestCommands(std::move(request.requestCommands))
        , socketContext(request.socketContext)
        , connectionState(request.connectionState)
        , transfereEncoding(request.transfereEncoding)
        , masterRequest(request.masterRequest) { // NOLINT
        request.init(headers["Host"]);
    }

    Request::~Request() {
        for (const RequestCommand* requestCommand : requestCommands) {
            delete requestCommand;
        }

        if (masterRequest.lock() && Sink::isStreaming()) {
            socketContext->streamEof();
        }
    }

    void Request::setMasterRequest(const std::shared_ptr<Request>& masterRequest) {
        this->masterRequest = masterRequest;
    }

    void Request::init(const std::string& host) {
        onResponseReceived = nullptr;
        onResponseParseError = nullptr;
        method = "GET";
        url = "/";
        httpMajor = 1;
        httpMinor = 1;
        queries.clear();
        headers.clear();
        cookies.clear();
        contentSent = 0;
        contentLength = 0;
        requestCommands.clear();
        connectionState = ConnectionState::Default;
        transfereEncoding = TransferEncoding::HTTP10;

        this->host(host);
        set("X-Powered-By", "snode.c");
    }

    Request& Request::host(const std::string& host) {
        set("Host", host);

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

            if (web::http::ciEquals(field, "Content-Length")) {
                contentLength = std::stoul(value);
            } else if (web::http::ciEquals(field, "Connection") && web::http::ciContains(headers[field], "close")) {
                connectionState = ConnectionState::Close;
            } else if (web::http::ciEquals(field, "Connection") && web::http::ciContains(headers[field], "keep-alive")) {
                connectionState = ConnectionState::Keep;
            }

            if (web::http::ciEquals(field, "Content-Length")) {
                contentLength = std::stoul(value);
                transfereEncoding = TransferEncoding::Identity;
                headers.erase("Transfer-Encoding");
            } else if (web::http::ciEquals(field, "Transfer-Encoding") && web::http::ciContains(headers[field], "chunked")) {
                transfereEncoding = TransferEncoding::Chunked;
                headers.erase("Content-Length");
            } else if (web::http::ciEquals(field, "Connection") && web::http::ciContains(headers[field], "close")) {
                connectionState = ConnectionState::Close;
            } else if (web::http::ciEquals(field, "Connection") && web::http::ciContains(headers[field], "keep-alive")) {
                connectionState = ConnectionState::Keep;
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

    void Request::responseParseError(const std::shared_ptr<Request>& request, const std::string& message) {
        LOG(TRACE) << "HTTP response parse error: " << request->method << " " << request->url << " "
                   << "HTTP/" << request->httpMajor << "." << request->httpMinor << ": " << message;
    }

    bool Request::send(const char* chunk,
                       std::size_t chunkLen,
                       const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                       const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        bool queued = true;

        if (masterRequest.lock()) {
            this->onResponseReceived = onResponseReceived;
            this->onResponseParseError = onResponseParseError;

            if (chunkLen > 0) {
                set("Content-Type", "application/octet-stream", false);
            }

            sendHeader();
            sendFragment(chunk, chunkLen);

            requestCommands.push_back(new commands::EndCommand());

            socketContext->requestPrepared(*this);
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

    bool Request::upgrade(const std::string& url,
                          const std::string& protocols,
                          const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                          const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        bool success = true;

        if (masterRequest.lock()) {
            this->onResponseReceived = onResponseReceived;
            this->onResponseParseError = onResponseParseError;

            requestCommands.push_back(new commands::UpgradeCommand(url, protocols));

            socketContext->requestPrepared(*this);
        } else {
            success = false;
        }

        return success;
    }

    bool Request::upgrade(const std::shared_ptr<Response>& response, const std::function<void(bool)>& status) {
        bool upgraded = false;

        core::socket::stream::SocketContext* newSocketContext = nullptr;

        if (masterRequest.lock()) {
            if (response != nullptr) {
                if (web::http::ciContains(response->get("connection"), "Upgrade")) {
                    web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory =
                        web::http::client::SocketContextUpgradeFactorySelector::instance()->select(*this, *response);

                    if (socketContextUpgradeFactory != nullptr) {
                        newSocketContext = socketContextUpgradeFactory->create(socketContext->getSocketConnection());
                        if (newSocketContext != nullptr) {
                            LOG(DEBUG) << "HTTP: SocketContextUpgrade created";
                            socketContext->switchSocketContext(newSocketContext);
                            upgraded = true;
                        } else {
                            LOG(DEBUG) << "HTTP: SocketContextUpgrade not created";
                            socketContext->close();
                        }
                    } else {
                        LOG(DEBUG) << "HTTP: SocketContextUpgradeFactory not existing";
                        socketContext->close();
                    }
                } else {
                    LOG(DEBUG) << "HTTP: Response did not contain upgrade";
                    socketContext->close();
                }
            } else {
                LOG(DEBUG) << "HTTP: Upgrade internal error: Response has gone away";
                socketContext->close();
            }
        } else {
            LOG(DEBUG) << "HTTP: Upgrade error: SocketContext has gone away";
        }

        status(newSocketContext != nullptr);

        return upgraded;
    }

    bool Request::sendFile(const std::string& file,
                           const std::function<void(int)>& onStatus,
                           const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                           const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        bool queued = true;

        if (masterRequest.lock()) {
            this->onResponseReceived = onResponseReceived;
            this->onResponseParseError = onResponseParseError;

            requestCommands.push_back(new commands::SendFileCommand(file, onStatus));

            socketContext->requestPrepared(*this);
        } else {
            queued = false;
        }

        return queued;
    }

    Request& Request::sendHeader() {
        if (masterRequest.lock()) {
            requestCommands.push_back(new commands::SendHeaderCommand());
        }

        return *this;
    }

    Request& Request::sendFragment(const char* chunk, std::size_t chunkLen) {
        if (masterRequest.lock()) {
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

        if (masterRequest.lock()) {
            this->onResponseReceived = onResponseReceived;
            this->onResponseParseError = onResponseParseError;

            sendHeader();

            requestCommands.push_back(new commands::EndCommand());

            socketContext->requestPrepared(*this);
        } else {
            queued = false;
        }

        return queued;
    }

    bool Request::execute() {
        bool success = false;
        bool commandError = false;

        if (masterRequest.lock()) {
            success = true;

            bool atomar = true;
            for (RequestCommand* requestCommand : requestCommands) {
                if (!commandError) {
                    const bool commandAtomar = requestCommand->execute(this);
                    if (atomar) {
                        atomar = commandAtomar;
                    }

                    commandError = requestCommand->getError();
                }

                delete requestCommand;
            }
            requestCommands.clear();

            if (atomar && !commandError) {
                requestSent();
            }
        }

        return success && !commandError;
    }

    bool Request::executeSendFile(const std::string& file, const std::function<void(int)>& onStatus) {
        bool error = true;

        std::string absolutFileName = file;

        if (std::filesystem::exists(absolutFileName)) {
            std::error_code ec;
            absolutFileName = std::filesystem::canonical(absolutFileName);

            if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                core::file::FileReader::open(absolutFileName)->pipe(this, [this, &error, &absolutFileName, &onStatus](int errnum) -> void {
                    errno = errnum;
                    onStatus(errnum);

                    if (errnum == 0) {
                        if (httpMajor == 1) {
                            error = false;

                            set("Content-Type", web::http::MimeTypes::contentType(absolutFileName), false);
                            set("Last-Modified", httputils::file_mod_http_date(absolutFileName), false);
                            if (httpMinor == 1) {
                                set("Transfer-Encoding", "chunked");
                            } else {
                                set("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName)));
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

        return error;
    }

    bool Request::executeUpgrade(const std::string& url, const std::string& protocols) {
        this->url = url;

        set("Connection", "Upgrade", true);
        set("Upgrade", protocols, true);

        web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory =
            web::http::client::SocketContextUpgradeFactorySelector::instance()->select(protocols, *this);

        if (socketContextUpgradeFactory != nullptr) {
            socketContextUpgradeFactory->checkRefCount();

            executeSendHeader();

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
        if (transfereEncoding == TransferEncoding::Chunked) {
            socketContext->sendToPeer(to_hex_str(chunkLen).append("\r\n"));
        }

        socketContext->sendToPeer(chunk, chunkLen);
        contentSent += chunkLen;

        if (transfereEncoding == TransferEncoding::Chunked) {
            socketContext->sendToPeer("\r\n");
            contentLength += chunkLen;
        }

        return true;
    }

    void Request::deliverResponse(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response) {
        onResponseReceived(request, response);
    }

    void Request::deliverResponseParseError(const std::shared_ptr<Request>& request, const std::string& message) {
        onResponseParseError(request, message);
    }

    void Request::requestSent() {
        if (transfereEncoding == TransferEncoding::Chunked) {
            executeSendFragment("", 0); // For transfere encoding chunked. Terminate the chunk sequence.
        }

        if (masterRequest.lock()) {
            socketContext->requestSent(contentSent == contentLength);
        }
    }

    void Request::onSourceConnect(core::pipe::Source* source) {
        if (masterRequest.lock() && socketContext->streamToPeer(source)) {
            source->start();
        } else {
            source->stop();
        }
    }

    void Request::onSourceData(const char* chunk, std::size_t chunkLen) {
        executeSendFragment(chunk, chunkLen);
    }

    void Request::onSourceEof() {
        if (masterRequest.lock()) {
            socketContext->streamEof();

            requestSent();
        }
    }

    void Request::onSourceError(int errnum) {
        errno = errnum;

        if (masterRequest.lock()) {
            socketContext->streamEof();
            socketContext->close();

            requestSent();
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
