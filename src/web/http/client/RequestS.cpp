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

#include "web/http/client/RequestS.h"

#include "core/socket/stream/SocketConnection.h"
#include "web/http/client/SocketContext.h"
#include "web/http/client/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    Request::Request(SocketContext* socketContext, const std::string& host)
        : hostFieldValue(host)
        , socketContext(socketContext) {
    }

    Request::Request(Request&& request) noexcept
        : hostFieldValue(request.hostFieldValue) // NOLINT
        , method(std::move(request.method))
        , url(std::move(request.url))
        , httpMajor(request.httpMajor)
        , httpMinor(request.httpMinor)
        , count(request.count)
        , queries(std::move(request.queries))
        , headers(std::move(request.headers))
        , cookies(std::move(request.cookies))
        , trailer(std::move(request.trailer))
        , contentLength(request.contentLength)
        , masterRequest(request.masterRequest) // NOLINT
        , socketContext(request.socketContext)
        , transferEncoding(request.transferEncoding)
        , connectionState(request.connectionState) {
        request.count++;
    }

    void Request::setMasterRequest(const std::shared_ptr<MasterRequest>& masterRequest) {
        this->masterRequest = masterRequest;
    }

    std::shared_ptr<MasterRequest> Request::getMasterRequest() const {
        return masterRequest.lock();
    }

    SocketContext* Request::getSocketContext() const {
        return socketContext;
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

    std::string Request::header(const std::string& field) const {
        return headers.contains(field) ? headers.find("field")->second : "";
    }

    const CiStringMap<std::string>& Request::getQueries() const {
        return queries;
    }

    const CiStringMap<std::string>& Request::getHeaders() const {
        return headers;
    }

    const CiStringMap<std::string>& Request::getTrailer() const {
        return trailer;
    }

    const CiStringMap<std::string>& Request::getCookies() const {
        return cookies;
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

} // namespace web::http::client
