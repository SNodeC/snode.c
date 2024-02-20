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
#include "web/http/http_utils.h"

//

#include "commands/EndCommand.h"
#include "commands/SendFileCommand.h"
#include "commands/SendFragmentCommand.h"
#include "commands/SendHeaderCommand.h"
#include "commands/UpgradeCommand.h"

namespace core::socket::stream {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <filesystem>
#include <system_error>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    Request::Request(web::http::client::SocketContext* clientContext)
        : socketContext(clientContext) {
    }

    Request::~Request() {
        if (socketContext != nullptr) {
            socketContext->streamEof();
        }

        for (const RequestCommand* requestCommand : requestCommands) {
            delete requestCommand;
        }
    }

    void Request::stopResponse() {
        stop();
        socketContext = nullptr;
    }

    void Request::init(const std::string& host) {
        method = "GET";
        url.clear();
        httpMajor = 1;
        httpMinor = 1;
        queries.clear();
        headers.clear();
        cookies.clear();
        contentLength = 0;
        contentSent = 0;
        connectionState = ConnectionState::Default;

        set("Host", host);
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

            if (field == "Content-Length") {
                contentLength = std::stoul(value);
            } else if (field == "Connection" && httputils::ci_contains(headers[field], "close")) {
                connectionState = ConnectionState::Close;
            } else if (field == "Connection" && httputils::ci_contains(headers[field], "keep-alive")) {
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

    void Request::responseParseError(const std::shared_ptr<Request>& request, const std::string& message) {
        LOG(INFO) << "HTTP Response parse error: " << request->url << " - " << message;
    }

    void Request::send(const char* junk,
                       std::size_t junkLen,
                       const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                       const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        this->onResponseReceived = onResponseReceived;
        this->onResponseParseError = onResponseParseError;

        if (socketContext != nullptr) {
            if (junkLen > 0) {
                set("Content-Type", "application/octet-stream");
            }
            set("Content-Length", std::to_string(junkLen), true);

            sendHeader();
            sendFragment(junk, junkLen);

            socketContext->requestPrepared(*this);
        }
    }

    void Request::send(const std::string& junk,
                       const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                       const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        if (!junk.empty()) {
            headers.insert({"Content-Type", "text/html; charset=utf-8"});
        }
        send(junk.data(), junk.size(), onResponseReceived, onResponseParseError);
    }

    void Request::upgrade(const std::string& url,
                          const std::string& protocols,
                          const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                          const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        this->onResponseReceived = onResponseReceived;
        this->onResponseParseError = onResponseParseError;

        if (socketContext != nullptr) {
            requestCommands.push_back(new commands::UpgradeCommand(url, protocols));

            socketContext->requestPrepared(*this);
        }
    }

    void Request::upgrade(const std::shared_ptr<Response>& response, const std::function<void(bool success)>& status) {
        core::socket::stream::SocketContext* newSocketContext = nullptr;

        if (socketContext != nullptr) {
            if (response != nullptr) {
                if (httputils::ci_contains(response->get("connection"), "Upgrade")) {
                    web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory =
                        web::http::client::SocketContextUpgradeFactorySelector::instance()->select(*this, *response);

                    if (socketContextUpgradeFactory != nullptr) {
                        newSocketContext = socketContextUpgradeFactory->create(socketContext->getSocketConnection());

                        if (newSocketContext != nullptr) {
                            socketContext->switchSocketContext(newSocketContext);
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
    }

    void Request::sendFile(const std::string& file,
                           const std::function<void(int)>& onStatus,
                           const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                           const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        this->onResponseReceived = onResponseReceived;
        this->onResponseParseError = onResponseParseError;

        if (socketContext != nullptr) {
            requestCommands.push_back(new commands::SendFileCommand(file, onStatus));

            socketContext->requestPrepared(*this);
        }
    }

    void Request::end(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                      const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError) {
        this->onResponseReceived = onResponseReceived;
        this->onResponseParseError = onResponseParseError;

        if (socketContext != nullptr) {
            sendHeader();

            requestCommands.push_back(new commands::EndCommand());

            socketContext->requestPrepared(*this);
        }
    }

    Request& Request::sendHeader() {
        if (socketContext != nullptr) {
            requestCommands.push_back(new commands::SendHeaderCommand());
        }

        return *this;
    }

    void Request::deliverResponse(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response) {
        onResponseReceived(request, response);
    }

    void Request::deliverResponseParseError(const std::shared_ptr<Request>& request, const std::string& message) {
        onResponseParseError(request, message);
    }

    Request& Request::sendFragment(const char* junk, std::size_t junkLen) {
        if (socketContext != nullptr) {
            requestCommands.push_back(new commands::SendFragmentCommand(junk, junkLen));
        }

        return *this;
    }

    Request& Request::sendFragment(const std::string& data) {
        return sendFragment(data.data(), data.size());
    }

    void Request::dispatchSendHeader() {
        if (socketContext != nullptr) {
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

            for (const auto& [field, value] : headers) {
                socketContext->sendToPeer(std::string(field).append(":").append(value).append("\r\n"));
            }

            for (const auto& [name, value] : cookies) {
                socketContext->sendToPeer(std::string("Cookie:").append(name).append("=").append(value).append("\r\n"));
            }

            socketContext->sendToPeer("\r\n");
        } else {
            LOG(DEBUG) << "HTTP: dispatchSendHeader error: SocketContext has gone away";
        }
    }

    void Request::dispatchSendFragment(const char* junk, std::size_t junkLen) {
        if (socketContext != nullptr) {
            socketContext->sendToPeer(junk, junkLen);
            contentSent += junkLen;
        } else {
            LOG(DEBUG) << "HTTP: Upgrade error: SocketContext has gone away";
        }
    }

    void Request::dispatchSendFile(const std::string& file, const std::function<void(int errnum)>& onResponseReceived) {
        if (socketContext != nullptr) {
            std::string absolutFileName = file;

            if (std::filesystem::exists(absolutFileName)) {
                std::error_code ec;
                absolutFileName = std::filesystem::canonical(absolutFileName);

                if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                    core::file::FileReader::open(absolutFileName)
                        ->pipe(this, [this, &absolutFileName, &onResponseReceived](core::pipe::Source* source, int errnum) -> void {
                            onResponseReceived(errnum);

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
                    onResponseReceived(errno);
                }
            } else {
                errno = ENOENT;
                onResponseReceived(errno);
            }
        } else {
            LOG(DEBUG) << "HTTP: Upgrade error: SocketContext has gone away";
        }
    }

    void Request::dispatchUpgrade(const std::string& url, const std::string& protocols) {
        this->url = url;

        set("Connection", "Upgrade", true);
        set("Upgrade", protocols, true);

        web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory =
            web::http::client::SocketContextUpgradeFactorySelector::instance()->select(protocols, *this);

        if (socketContextUpgradeFactory != nullptr) {
            socketContextUpgradeFactory->checkRefCount();

            dispatchSendHeader();
        } else {
            socketContext->close();
        }

        dispatchCompleted();
    }

    void Request::dispatchEnd() {
        dispatchCompleted();
    }

    void Request::dispatchCompleted() {
        if (socketContext != nullptr) {
            socketContext->requestSent(contentSent == contentLength);
        }
    }

    void Request::onSourceConnect(core::pipe::Source* source) {
        if (socketContext != nullptr && socketContext->streamToPeer(source)) {
            dispatchSendHeader();

            source->start();
        } else {
            source->stop();
        }
    }

    void Request::onSourceData(const char* junk, std::size_t junkLen) {
        dispatchSendFragment(junk, junkLen);
    }

    void Request::onSourceEof() {
        if (socketContext != nullptr) {
            socketContext->streamEof();
        }

        dispatchCompleted();
    }

    void Request::onSourceError(int errnum) {
        errno = errnum;

        if (socketContext != nullptr) {
            socketContext->streamEof();
            socketContext->close();
        }

        dispatchCompleted();
    }

    const std::string& Request::header(const std::string& field) {
        return headers[field];
    }

    SocketContext* Request::getSocketContext() const {
        return socketContext;
    }

    void Request::execute() {
        for (RequestCommand* requestCommand : requestCommands) {
            requestCommand->execute(this);
            delete requestCommand;
        }
        requestCommands.clear();
    }

} // namespace web::http::client
