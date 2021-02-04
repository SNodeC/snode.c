/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "Logger.h" // for Writer, Storage, VLOG
#include "Parser.h"
#include "client/ResponseParser.h" // for HTTPResponseParser, ResponseCookie
#include "server/RequestParser.h"  // for RequestParser

#include <cstddef>
#include <cstring>     // for memcpy, std::size_t
#include <functional>  // for function
#include <map>         // for map
#include <string>      // for allocator, string, operator+, char_t...
#include <type_traits> // for add_const<>::type
#include <utility>     // for pair, tuple_element<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http {
    class CookieOptions;
}

using namespace http;

int main(int argc, char* argv[]) {
    Logger::init(argc, argv);

    server::RequestParser requestParser(
        [](void) -> void {
        },
        [](const std::string& method,
           const std::string& originalUrl,
           const std::string& httpVersion,
           int httpMajor,
           int httpMinor,
           const std::map<std::string, std::string>& queries) -> void {
            VLOG(0) << "++ Request: " << method << " " << originalUrl << " "
                    << " " << httpVersion << " " << httpMajor << " " << httpMinor;
            for (const std::pair<std::string, std::string>& query : queries) {
                VLOG(0) << "++    Query: " << query.first << " = " << query.second;
            }
        },
        [](const std::map<std::string, std::string>& header, const std::map<std::string, std::string>& cookies) -> void {
            VLOG(0) << "++    Header: ";
            for (const std::pair<std::string, std::string>& headerField : header) {
                VLOG(0) << "++      " << headerField.first << " = " << headerField.second;
            }
            VLOG(0) << "++    Cookie: ";
            for (const std::pair<std::string, std::string>& cookie : cookies) {
                VLOG(0) << "++      " << cookie.first << " = " << cookie.second;
            }
        },
        [](char* content, std::size_t contentLength) -> void {
            char* strContent = new char[contentLength + 1];
            memcpy(strContent, content, contentLength);
            strContent[contentLength] = 0;
            VLOG(0) << "++    OnContent: " << contentLength << " : " << strContent;
            delete[] strContent;
        },
        []() -> void {
            VLOG(0) << "++    OnParsed";
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "++    OnError: " << status << " : " << reason;
        });

    std::string httpRequest = "GET /admin/new/index.html?hihihi=3343&query=2324#fragment HTTP/1.1\r\n"
                              "Field1: Value1\r\n"
                              "Field2: Field2\r\n"
                              "Field2: Field3\r\n" // is allowed and must be combined with a comma as separator
                              "Content-Length: 8\r\n"
                              "Cookie: MyCookie1=MyValue1; MyCookie2=MyValue2\r\n"
                              "Cookie: MyCookie3=MyValue3\r\n"
                              "Cookie: MyCookie4=MyValue4\r\n"
                              "Cookie: MyCookie5=MyValue5; MyCookie6=MyValue6\r\n"
                              "\r\n"
                              "juhuhuhu";

    VLOG(0) << "==================================";
    VLOG(0) << httpRequest;
    VLOG(0) << "----------------------------------";

    requestParser.parse(httpRequest.c_str(), httpRequest.size());
    requestParser.reset();

    VLOG(0) << "==================================";
    VLOG(0) << httpRequest;
    VLOG(0) << "----------------------------------";

    requestParser.parse(httpRequest.c_str(), httpRequest.size());
    requestParser.reset();

    client::ResponseParser responseParser(
        [](void) -> void {
        },
        [](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
            VLOG(0) << "++ Response: " << httpVersion << " " << statusCode << " " << reason;
        },
        [](const std::map<std::string, std::string>& headers, const std::map<std::string, CookieOptions>& cookies) -> void {
            VLOG(0) << "++   Headers:";
            for (const auto& [field, value] : headers) {
                VLOG(0) << "++       " << field + " = " + value;
            }

            VLOG(0) << "++   Cookies:";
            for (const auto& [name, cookie] : cookies) {
                VLOG(0) << "++     " + name + " = " + cookie.getValue();
                for (const auto& [option, value] : cookie.getOptions()) {
                    VLOG(0) << "++       " + option + " = " + value;
                }
            }
        },
        [](char* content, std::size_t contentLength) -> void {
            char* strContent = new char[contentLength + 1];
            memcpy(strContent, content, contentLength);
            strContent[contentLength] = 0;
            VLOG(0) << "++   OnContent: " << contentLength << " : " << strContent;
            delete[] strContent;
        },
        [](client::ResponseParser& parser) -> void {
            VLOG(0) << "++   OnParsed";
            parser.reset();
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "++   OnError: " + std::to_string(status) + " - " + reason;
        });

    std::string httpResponse = "HTTP/1.1 200 OK\r\n"
                               "Field: Value\r\n"
                               "Set-Cookie: CookieName = CookieValue; OptionName = OptionValue\r\n"
                               "Set-Cookie: CookieName1 = CookieValue1; OptionName1 = OptionValue1; OptionName2 = OptionValue2;\r\n"
                               "Content-Length: 8\r\n"
                               "\r\n"
                               "juhuhuhu";

    VLOG(0) << "==================================";
    VLOG(0) << httpResponse;
    VLOG(0) << "----------------------------------";
    responseParser.parse(httpResponse.c_str(), httpResponse.size());
    responseParser.reset();

    VLOG(0) << "==================================";
    VLOG(0) << httpResponse;
    VLOG(0) << "----------------------------------";
    responseParser.parse(httpResponse.c_str(), httpResponse.size());
    responseParser.reset();

    return 0;
}
