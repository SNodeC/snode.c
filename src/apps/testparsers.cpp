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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "web/http/CookieOptions.h"
#include "web/http/client/ResponseParser.h"
#include "web/http/server/RequestParser.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace web::http;

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    logger::Logger::init();

    server::RequestParser requestParser(
        nullptr,
        []() -> void {
        },
        [](const std::string& method,
           const std::string& originalUrl,
           const std::string& httpVersion,
           int httpMajor,
           int httpMinor,
           const std::map<std::string, std::string>& queries) -> void {
            VLOG(0) << "++ Request: " << method << " " << originalUrl << " "
                    << " " << httpVersion << " " << httpMajor << " " << httpMinor;
            for (const auto& [queryField, queryValue] : queries) {
                VLOG(0) << "++    Query: " << queryField << " = " << queryValue;
            }
        },
        [](std::map<std::string, std::string>& header, std::map<std::string, std::string>& cookies) -> void {
            VLOG(0) << "++    Header: ";
            for (auto& [headerField, headerFieldValue] : header) {
                VLOG(0) << "++      " << headerField << " = " << headerFieldValue;
            }
            VLOG(0) << "++    Cookie: ";
            for (auto& [cookieName, cookieValue] : cookies) {
                VLOG(0) << "++      " << cookieName << " = " << cookieValue;
            }
        },
        [](std::vector<uint8_t>& content) -> void {
            content.push_back(0);
            VLOG(0) << content.data();
        },
        []() -> void {
            VLOG(0) << "++    OnParsed";
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "++    OnError: " << status << " : " << reason;
        });

    const std::string httpRequest = "GET /admin/new/index.html?hihihi=3343&query=2324#fragment HTTP/1.1\r\n"
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

    requestParser.parse();
    requestParser.reset();

    VLOG(0) << "==================================";
    VLOG(0) << httpRequest;
    VLOG(0) << "----------------------------------";

    requestParser.parse();
    requestParser.reset();

    client::ResponseParser responseParser(
        nullptr,
        []() -> void {
        },
        [](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
            VLOG(0) << "++ Response: " << httpVersion << " " << statusCode << " " << reason;
        },
        [](std::map<std::string, std::string>& headers, std::map<std::string, web::http::CookieOptions>& cookies) -> void {
            VLOG(0) << "++   Headers:";
            for (auto& [field, value] : headers) {
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
        [](std::vector<uint8_t>& content) -> void {
            content.push_back(0);
            VLOG(0) << content.data();
        },
        [](client::ResponseParser& parser) -> void {
            VLOG(0) << "++   OnParsed";
            parser.reset();
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "++   OnError: " + std::to_string(status) + " - " + reason;
        });

    const std::string httpResponse = "HTTP/1.1 200 OK\r\n"
                                     "Field: Value\r\n"
                                     "Set-Cookie: CookieName = CookieValue; OptionName = OptionValue\r\n"
                                     "Set-Cookie: CookieName1 = CookieValue1; OptionName1 = OptionValue1; OptionName2 = OptionValue2;\r\n"
                                     "Content-Length: 8\r\n"
                                     "\r\n"
                                     "juhuhuhu";

    VLOG(0) << "==================================";
    VLOG(0) << httpResponse;
    VLOG(0) << "----------------------------------";
    responseParser.parse();
    responseParser.reset();

    VLOG(0) << "==================================";
    VLOG(0) << httpResponse;
    VLOG(0) << "----------------------------------";
    responseParser.parse();
    responseParser.reset();

    return 0;
}
