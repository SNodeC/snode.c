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
#include <tuple> // for tie, tuple

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPResponseParser.h"
#include "httputils.h"

namespace http {

    HTTPResponseParser::HTTPResponseParser(
        const std::function<void(const std::string&, const std::string&, const std::string&)>& onResponse,
        const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, ResponseCookie>&)>& onHeader,
        const std::function<void(char*, size_t)>& onContent, const std::function<void(void)>& onParsed,
        const std::function<void(int status, const std::string& reason)>& onError)
        : onResponse(onResponse)
        , onHeader(onHeader)
        , onContent(onContent)
        , onParsed(onParsed)
        , onError(onError) {
    }

    HTTPResponseParser::HTTPResponseParser(
        const std::function<void(const std::string&, const std::string&, const std::string&)>&& onResponse,
        const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, ResponseCookie>&)>&& onHeader,
        const std::function<void(char*, size_t)>&& onContent, const std::function<void(void)>&& onParsed,
        const std::function<void(int status, const std::string& reason)>&& onError)
        : onResponse(onResponse)
        , onHeader(onHeader)
        , onContent(onContent)
        , onParsed(onParsed)
        , onError(onError) {
    }

    void HTTPResponseParser::reset() {
        HTTPParser::reset();
        httpVersion.clear();
        statusCode.clear();
        reason.clear();
        cookies.clear();
    }

    enum HTTPParser::PAS HTTPResponseParser::parseStartLine(std::string& line) {
        enum HTTPParser::PAS PAS = HTTPParser::PAS::HEADER;

        if (!line.empty()) {
            std::string remaining;

            std::tie(httpVersion, remaining) = httputils::str_split(line, ' '); // if split not found second will be empty
            std::tie(statusCode, remaining) = httputils::str_split(remaining, ' ');
            std::tie(reason, std::ignore) = httputils::str_split(remaining, ' ');

            onResponse(httpVersion, statusCode, reason);
        } else {
            PAS = parsingError(400, "Response-line empty");
        }
        return PAS;
    }

    enum HTTPParser::PAS HTTPResponseParser::parseHeader() {
        for (auto& [field, value] : HTTPParser::headers) {
            VLOG(1) << "++ Parse header field: " << field << " = " << value;
            if (field != "set-cookie") {
                if (field == "content-length") {
                    HTTPParser::contentLength = std::stoi(value);
                }
            } else {
                std::string cookiesLine = value;

                while (!cookiesLine.empty()) {
                    std::string cookieLine;
                    std::tie(cookieLine, cookiesLine) = httputils::str_split(cookiesLine, ',');

                    std::string cookieOptions;
                    std::string cookie;
                    std::tie(cookie, cookieOptions) = httputils::str_split(cookieLine, ';');

                    std::string name;
                    std::string value;
                    std::tie(name, value) = httputils::str_split(cookie, '=');
                    httputils::str_trimm(name);
                    httputils::str_trimm(value);

                    VLOG(1) << "++ Cookie: " << name << " = " << value;

                    std::map<std::string, ResponseCookie>::iterator cookieElement;
                    bool inserted;
                    std::tie(cookieElement, inserted) = cookies.insert({name, ResponseCookie(value)});

                    while (!cookieOptions.empty()) {
                        std::string option;
                        std::tie(option, cookieOptions) = httputils::str_split(cookieOptions, ';');

                        std::string name;
                        std::string value;
                        std::tie(name, value) = httputils::str_split(option, '=');
                        httputils::str_trimm(name);
                        httputils::str_trimm(value);

                        VLOG(1) << "    ++ CookieOption: " << name << " = " << value;
                        cookieElement->second.setOption(name, value);
                    }
                }
            }
        }

        onHeader(HTTPParser::headers, cookies);

        enum HTTPParser::PAS PAS = HTTPParser::PAS::BODY;
        if (contentLength == 0) {
            parsingFinished();
            PAS = PAS::FIRSTLINE;
        }

        return PAS;
    }

    enum HTTPParser::PAS HTTPResponseParser::parseContent(char* content, size_t size) {
        onContent(content, size);
        parsingFinished();

        return PAS::FIRSTLINE;
    }

    enum HTTPParser::PAS HTTPResponseParser::parsingError(int code, const std::string& reason) {
        onError(code, reason);
        reset();

        return PAS::ERROR;
    }

    void HTTPResponseParser::parsingFinished() {
        onParsed();
    }

} // namespace http
