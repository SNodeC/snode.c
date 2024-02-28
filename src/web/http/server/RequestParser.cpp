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

#include "web/http/server/RequestParser.h"

#include "web/http/decoder/Chunk.h"
#include "web/http/decoder/HTTP10.h"
#include "web/http/decoder/Identity.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <regex>
#include <tuple>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    RequestParser::RequestParser(core::socket::stream::SocketContext* socketContext,
                                 const std::function<void(Request&&)>& onParsed,
                                 const std::function<void(int, const std::string&)>& onError)
        : Parser(socketContext)
        , onParsed(onParsed)
        , onError(onError) {
    }

    bool RequestParser::methodSupported(const std::string& method) const {
        return supportedMethods.contains(method);
    }

    void RequestParser::begin() {
    }

    Parser::ParserState RequestParser::parseStartLine(const std::string& line) {
        Parser::ParserState parserState = Parser::ParserState::HEADER;

        if (!line.empty()) {
            std::string remaining;

            std::tie(request.method, remaining) = httputils::str_split(line, ' ');
            std::tie(request.url, request.httpVersion) = httputils::str_split(remaining, ' ');

            std::string queriesLine;
            std::tie(std::ignore, queriesLine) = httputils::str_split(request.url, '?');

            if (!methodSupported(request.method)) {
                parserState = parsingError(400, "Bad request method");
            } else if (request.url.empty() || request.url.front() != '/') {
                parserState = parsingError(400, "Malformed request");
            } else {
                std::smatch httpVersionMatch;
                if (!std::regex_match(request.httpVersion, httpVersionMatch, httpVersionRegex)) {
                    parserState = parsingError(400, "Wrong protocol-version");
                } else {
                    request.httpMajor = std::stoi(httpVersionMatch.str(1));
                    request.httpMinor = std::stoi(httpVersionMatch.str(2));

                    while (!queriesLine.empty()) {
                        std::string query;

                        std::tie(query, queriesLine) = httputils::str_split(queriesLine, '&');
                        request.queries.insert(httputils::str_split(query, '='));
                    }
                }
            }
        } else {
            parserState = parsingError(400, "Request-line empty");
        }

        return parserState;
    }

    Parser::ParserState RequestParser::parseHeader() {
        for (auto& [field, value] : Parser::headers) {
            if (!web::http::ciEquals(field, "Cookie")) {
                if (web::http::ciEquals(field, "Connection")) {
                    if (web::http::ciContains(headers[field], "keep-alive")) {
                        request.connectionState = ConnectionState::Keep;
                    } else if (web::http::ciContains(headers[field], "close")) {
                        request.connectionState = ConnectionState::Close;
                    }
                } else if (web::http::ciEquals(field, "Content-Length")) {
                    contentLength = std::stoul(value);
                    request.transferEncoding = TransferEncoding::Identity;
                    headers.erase("Transfer-Encoding");

                    decoderQueue.emplace_back(new web::http::decoder::Identity(socketContext, contentLength));
                } else if (web::http::ciEquals(field, "Transfer-Encoding")) {
                    if (web::http::ciContains(headers[field], "chunked")) {
                        request.transferEncoding = TransferEncoding::Chunked;
                        headers.erase("Content-Length");

                        decoderQueue.emplace_back(new web::http::decoder::Chunk(socketContext));
                    }
                    if (web::http::ciContains(headers[field], "compressed")) {
                        //  decoderQueue.emplace_back(new web::http::decoder::Compress(socketContext));
                    }
                    if (web::http::ciContains(headers[field], "deflate")) {
                        //  decoderQueue.emplace_back(new web::http::decoder::Deflate(socketContext));
                    }
                    if (web::http::ciContains(headers[field], "gzip")) {
                        //  decoderQueue.emplace_back(new web::http::decoder::GZip(socketContext));
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
                std::string cookiesLine = value;

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

                        request.cookies.insert({cookieName, cookieValue});
                    }
                }
            }
        }

        httpMajor = request.httpMajor;
        httpMinor = request.httpMinor;
        Parser::headers.erase("Cookie");
        request.headers = std::move(Parser::headers);

        if (decoderQueue.empty()) {
            if (request.httpMajor == 1 && request.httpMinor == 0) {
                decoderQueue.emplace_back(new web::http::decoder::HTTP10(socketContext));
                request.transferEncoding = TransferEncoding::HTTP10;
            } else if (request.httpMajor == 1 && request.httpMinor == 1) {
                decoderQueue.emplace_back(new web::http::decoder::Identity(socketContext, 0));
                request.transferEncoding = TransferEncoding::Identity;
            }
        }

        enum Parser::ParserState parserState = Parser::ParserState::BODY;
        if (request.transferEncoding == TransferEncoding::Identity && contentLength == 0) {
            parserState = parsingFinished();
        }

        return parserState;
    }

    Parser::ParserState RequestParser::parseContent(std::vector<uint8_t>& content) {
        request.body = std::move(content);

        return parsingFinished();
    }

    Parser::ParserState RequestParser::parsingFinished() {
        onParsed(std::move(request));

        reset();

        return ParserState::BEGIN;
    }

    Parser::ParserState RequestParser::parsingError(int code, const std::string& reason) {
        onError(code, reason + "\r\n");

        reset();

        return ParserState::ERROR;
    }

} // namespace web::http::server
