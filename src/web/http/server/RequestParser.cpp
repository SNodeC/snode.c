/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "web/http/server/RequestParser.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <regex>
#include <tuple>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    RequestParser::RequestParser(core::socket::stream::SocketContext* socketContext,
                                 const std::function<void()>& onRequestStart,
                                 const std::function<void(Request&&)>& onRequestParsed,
                                 const std::function<void(int, const std::string&)>& onRequestParseError)
        : Parser(socketContext)
        , onRequestStart(onRequestStart)
        , onRequestParsed(onRequestParsed)
        , onRequestParseError(onRequestParseError) {
    }

    bool RequestParser::methodSupported(const std::string& method) const {
        return supportedMethods.contains(method);
    }

    void RequestParser::begin() {
        onRequestStart();
    }

    void RequestParser::parseStartLine(const std::string& line) {
        parserState = Parser::ParserState::HEADER;

        if (!line.empty()) {
            std::string remaining;

            std::tie(request.method, remaining) = httputils::str_split(line, ' ');
            std::tie(request.url, request.httpVersion) = httputils::str_split(remaining, ' ');

            std::string rawPath;
            std::string queriesLine;
            std::tie(rawPath, queriesLine) = httputils::str_split(request.url, '?');

            std::string decodedPath;
            if (!httputils::url_decode(rawPath, decodedPath, httputils::UrlDecodeMode::Path)) {
                parseError(400, "Malformed URL path encoding");
                return;
            }

            if (!methodSupported(request.method)) {
                parseError(405, "Bad request method: " + request.method);
            } else if (request.url.empty() || request.url.front() != '/') {
                parseError(400, "Malformed request");
            } else {
                std::smatch httpVersionMatch;
                if (!std::regex_match(request.httpVersion, httpVersionMatch, httpVersionRegex)) {
                    parseError(505, "Wrong protocol-version: " + request.httpVersion);
                } else {
                    httpMajor = std::stoi(httpVersionMatch.str(1));
                    httpMinor = std::stoi(httpVersionMatch.str(2));

                    if (httpMinor == 0) {
                        transferEncoding = TransferEncoding::HTTP10;
                    } else if (httpMinor == 1) {
                        transferEncoding = TransferEncoding::Identity;
                    }

                    while (!queriesLine.empty()) {
                        std::string query;

                        std::tie(query, queriesLine) = httputils::str_split(queriesLine, '&');
                        const auto [rawKey, rawVal] = httputils::str_split(query, '=');
                        std::string key;
                        std::string val;
                        if (!httputils::url_decode(rawKey, key, httputils::UrlDecodeMode::Query) ||
                            !httputils::url_decode(rawVal, val, httputils::UrlDecodeMode::Query)) {
                            parseError(400, "Malformed URL encoding");
                            return;
                        }
                        if (!key.empty()) {
                            request.queries.insert_or_assign(key, val); // last value wins
                        }
                    }
                }
            }
        } else {
            parseError(400, "Request-line empty");
        }
    }

    bool RequestParser::allowsCloseDelimitedBody() const {
        return false;
    }

    void RequestParser::analyzeHeader() {
        if (httpMinor == 1) {
            if (!headers.contains("Host")) {
                parseError(400, "Missing Host");
                return;
            }
            std::string host = headers["Host"];
            httputils::str_trimm(host);
            if (host.empty() || host.find(',') != std::string::npos) {
                parseError(400, "Invalid Host");
                return;
            }
        }

        if (headers.contains("Transfer-Encoding")) {
            if (headers.contains("Content-Length")) {
                parseError(400, "Transfer-Encoding with Content-Length");
                return;
            }

            const std::vector<std::string> codings = httputils::splitCommaSeparatedTokens(headers["Transfer-Encoding"]);
            if (codings.empty()) {
                parseError(400, "Invalid Transfer-Encoding");
                return;
            }

            std::size_t chunkedCount = 0;
            for (std::size_t i = 0; i < codings.size(); ++i) {
                std::string coding = codings[i];
                std::tie(coding, std::ignore) = httputils::str_split(coding, ';');
                httputils::str_trimm(coding);

                if (coding.empty()) {
                    parseError(400, "Invalid Transfer-Encoding");
                    return;
                }

                if (httputils::tokenEquals(coding, "chunked")) {
                    ++chunkedCount;
                    if (i + 1 != codings.size()) {
                        parseError(400, "Chunked Transfer-Encoding not final");
                        return;
                    }
                } else {
                    parseError(501, "Unsupported Transfer-Encoding");
                    return;
                }
            }

            if (chunkedCount != 1 || codings.size() != 1) {
                parseError(400, "Invalid Transfer-Encoding");
                return;
            }

            useChunkedBodyDecoder();
        } else {
            std::size_t length = 0;
            switch (httputils::parseContentLength(headers, length)) {
                case httputils::ContentLengthParseResult::Absent:
                    break;
                case httputils::ContentLengthParseResult::Valid:
                    useIdentityBodyDecoder(length);
                    break;
                case httputils::ContentLengthParseResult::Invalid:
                    parseError(400, "Invalid Content-Length");
                    return;
            }
        }

        if (parserState == Parser::ParserState::ERROR) {
            return;
        }

        if (headers.contains("Connection")) {
            const std::string& connection = headers["Connection"];
            if (httputils::headerHasToken(connection, "close")) {
                request.connectionState = ConnectionState::Close;
            } else if (httputils::headerHasToken(connection, "keep-alive")) {
                request.connectionState = ConnectionState::Keep;
            }
        }

        if (headers.contains("Cookie")) {
            std::string& cookiesLine = headers["Cookie"];

            while (!cookiesLine.empty()) {
                std::string cookieLine;
                std::tie(cookieLine, cookiesLine) = httputils::str_split(cookiesLine, ',');

                while (!cookieLine.empty()) {
                    std::string cookie;
                    std::tie(cookie, cookieLine) = httputils::str_split(cookieLine, ';');

                    std::string cookieName;
                    std::string cookieValue;
                    std::tie(cookieName, cookieValue) = httputils::str_split(cookie, '=');

                    httputils::str_trimm(cookieName);
                    httputils::str_trimm(cookieValue);

                    request.cookies.emplace(cookieName, cookieValue);
                }
            }

            headers.erase("Cookie");
        }

        if (transferEncoding != TransferEncoding::Chunked && contentLength == 0) {
            parsingFinished();
        } else {
            parserState = Parser::ParserState::BODY;
        }
    }

    void RequestParser::parsingFinished() {
        request.httpMajor = httpMajor;
        request.httpMinor = httpMinor;
        request.headers = std::move(Parser::headers);
        request.body = std::move(content);

        onRequestParsed(std::move(request));

        reset();

        parserState = ParserState::BEGIN;
    }

    void RequestParser::parseError(int code, const std::string& reason) {
        onRequestParseError(code, reason);

        reset();

        parserState = ParserState::ERROR;
    }

} // namespace web::http::server
