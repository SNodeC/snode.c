/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <easylogging++.h>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPClientContext.h"
#include "socket/SocketConnectionBase.h"

namespace http {

    HTTPClientContext::HTTPClientContext(net::socket::SocketConnectionBase* socketConnection,
                                         const std::function<void(ClientResponse&)>& onResponse,
                                         const std::function<void(int status, const std::string& reason)>& onError)
        : socketConnection(socketConnection)
        , onResponse(onResponse)
        , onError(onError)
        , parser(
              [this](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
                  VLOG(0) << "++ Response: " << httpVersion << " " << statusCode << " " << reason;
                  clientResponse.httpVersion = httpVersion;
                  clientResponse.statusCode = statusCode;
                  clientResponse.reason = reason;
              },
              [this](const std::map<std::string, std::string>& headers,
                     const std::map<std::string, http::ResponseCookie>& cookies) -> void {
                  VLOG(0) << "++   Headers:";
                  for (auto [field, value] : headers) {
                      VLOG(0) << "++       " << field + " = " + value;
                  }

                  VLOG(0) << "++   Cookies:";
                  for (auto [name, cookie] : cookies) {
                      VLOG(0) << "++     " + name + " = " + cookie.getValue();
                      for (auto [option, value] : cookie.getOptions()) {
                          VLOG(0) << "++       " + option + " = " + value;
                      }
                  }

                  clientResponse.headers = &headers;
                  clientResponse.cookies = &cookies;
              },
              [this](char* content, size_t contentLength) -> void {
                  char* strContent = new char[contentLength + 1];
                  memcpy(strContent, content, contentLength);
                  strContent[contentLength] = 0;
                  VLOG(0) << "++   OnContent: " << contentLength << std::endl << strContent;
                  delete[] strContent;

                  clientResponse.body = content;
                  clientResponse.contentLength = contentLength;
              },
              [this](http::HTTPResponseParser& parser) -> void {
                  VLOG(0) << "++   OnParsed";
                  this->onResponse(clientResponse);
                  parser.reset();
              },
              [this](int status, const std::string& reason) -> void {
                  VLOG(0) << "++   OnError: " + std::to_string(status) + " - " + reason;
                  this->onError(status, reason);
              }) {
    }

    void HTTPClientContext::receiveResponseData(const char* junk, size_t junkLen) {
        parser.parse(junk, junkLen);
    }

} // namespace http
