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

#include "HTTPRequestParser.h"
#include "HTTPResponseParser.h"
#include "Logger.h"

#include <easylogging++.h>
#include <iostream>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c_-_server.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem"
#define KEYFPASS "snode.c"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    Logger::init(argc, argv);

    http::HTTPRequestParser requestParser(
        [](const std::string& method, const std::string& originalUrl, const std::string& fragment, const std::string& httpVersion,
           [[maybe_unused]] const std::map<std::string, std::string>& queries) -> void {
            VLOG(0) << "++ Request: " << method << " " << originalUrl << " "
                    << " " << httpVersion;
            for (std::pair<std::string, std::string> query : queries) {
                VLOG(0) << "++    Query: " << query.first << " = " << query.second;
            }
            VLOG(0) << "++    Fragment: " << fragment;
        },
        [](const std::map<std::string, std::string>& header, const std::map<std::string, std::string>& cookies) -> void {
            VLOG(0) << "++    Header: ";
            for (std::pair<std::string, std::string> headerField : header) {
                VLOG(0) << "++      " << headerField.first << " = " << headerField.second;
            }
            VLOG(0) << "++    Cookie: ";
            for (std::pair<std::string, std::string> cookie : cookies) {
                VLOG(0) << "++      " << cookie.first << " = " << cookie.second;
            }
        },
        [](char* content, size_t contentLength) -> void {
            char* strContent = new char[contentLength + 1];
            memcpy(strContent, content, contentLength);
            strContent[contentLength] = 0;
            VLOG(0) << "++    OnContent: " << contentLength << " : " << strContent;
            delete[] strContent;
        },
        [](void) -> void {
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

    http::HTTPResponseParser responseParser(
        [](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
            VLOG(0) << "++ Response: " << httpVersion << " " << statusCode << " " << reason;
        },
        [](const std::map<std::string, std::string>& headers, const std::map<std::string, http::ResponseCookie>& cookies) -> void {
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
        },
        [](char* content, size_t contentLength) -> void {
            char* strContent = new char[contentLength + 1];
            memcpy(strContent, content, contentLength);
            strContent[contentLength] = 0;
            VLOG(0) << "++   OnContent: " << contentLength << " : " << strContent;
            delete[] strContent;
        },
        []() -> void {
            VLOG(0) << "++   OnParsed";
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

    //    return simpleWebserver();
}
