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

#include "web/http/Parser.h"

#include "core/socket/stream/SocketContext.h"
#include "web/http/decoder/Chunked.h"
#include "web/http/decoder/HTTP10Response.h"
#include "web/http/decoder/Identity.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <algorithm>
#include <cctype>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_CONTENT_JUNK_LEN
#define MAX_CONTENT_JUNK_LEN 16384
#endif

namespace web::http {

    // HTTP/1.0 and HTTP/1.1
    const std::regex Parser::httpVersionRegex("^HTTP/([1])[.]([0-1])$");

    Parser::Parser(core::socket::stream::SocketContext* socketContext, const enum Parser::HTTPCompliance& compliance)
        : hTTPCompliance(compliance)
        , socketContext(socketContext) {
    }

    Parser::~Parser() {
        reset();
    }

    void Parser::reset() {
        parserState = ParserState::BEGIN;
        contentLength = 0;
        contentRead = 0;
        httpMinor = 0;
        httpMajor = 0;

        for (ContentDecoder* contentDecoder : decoderQueue) {
            delete contentDecoder;
        }

        decoderQueue.clear();
    }

    std::size_t Parser::parse() {
        std::size_t ret = 0;
        std::size_t consumed = 0;

        do {
            switch (parserState) {
                case ParserState::BEGIN:
                    begin();
                    parserState = ParserState::FIRSTLINE;
                    [[fallthrough]];
                case ParserState::FIRSTLINE:
                    ret = readStartLine();
                    break;
                case ParserState::HEADER:
                    ret = readHeaderLine();
                    break;
                case ParserState::BODY:
                    ret = readContent();
                    break;
                case ParserState::ERROR:
                    break;
            }
            consumed += ret;
        } while (ret > 0 && parserState != ParserState::BEGIN && parserState != ParserState::ERROR);

        return consumed;
    }

    std::size_t Parser::readStartLine() {
        std::size_t consumed = 0;
        std::size_t ret = 0;

        do {
            char ch = 0;
            ret = socketContext->readFromPeer(&ch, 1);

            if (ret > 0) {
                consumed += ret;
                if (ch == '\r' || ch == '\n') {
                    if (ch == '\n') {
                        parserState = parseStartLine(line);
                        line.clear();
                    }
                } else {
                    line += ch;
                }
            }
        } while (ret > 0 && parserState == ParserState::FIRSTLINE);

        return consumed;
    }

    std::size_t Parser::readHeaderLine() {
        std::size_t consumed = 0;
        std::size_t ret = 0;

        do {
            char ch = 0;
            ret = socketContext->readFromPeer(&ch, 1);

            if (ret > 0) {
                consumed++;

                if (ch == '\r' || ch == '\n') {
                    if (ch == '\n') {
                        if (EOL || line.empty()) {
                            if (!line.empty()) {
                                splitHeaderLine(line);
                                line.clear();
                            }
                            if (parserState != ParserState::ERROR) {
                                parserState = analyzeHeaders();
                            }
                            EOL = false;
                        } else {
                            EOL = true;
                        }
                    }
                } else if (EOL) {
                    EOL = false;

                    if (std::isblank(ch) != 0) {
                        if ((hTTPCompliance & HTTPCompliance::RFC7230) == HTTPCompliance::RFC7230) {
                            parserState = parsingError(400, "Header Folding");
                        } else {
                            line += ch;
                        }
                    } else {
                        splitHeaderLine(line);
                        line.clear();
                        line += ch;
                    }
                } else {
                    line += ch;
                }
            }
        } while (ret > 0 && parserState == ParserState::HEADER);

        return consumed;
    }

    void Parser::splitHeaderLine(const std::string& line) {
        if (!line.empty()) {
            auto [headerFieldName, value] = httputils::str_split(line, ':');

            if (headerFieldName.empty()) {
                parserState = parsingError(400, "Header-field empty");
            } else if ((std::isblank(headerFieldName.back()) != 0) || (std::isblank(headerFieldName.front()) != 0)) {
                parserState = parsingError(400, "White space before or after header-field");
            } else if (value.empty()) {
                parserState = parsingError(400, "Header-value of field \"" + headerFieldName + "\" empty");
            } else {
                httputils::str_trimm(value);

                if (headers.find(headerFieldName) == headers.end()) {
                    headers.emplace(headerFieldName, value);
                } else {
                    headers[headerFieldName] += "," + value;
                }
            }
        } else {
            parserState = parsingError(400, "Header-line empty");
        }
    }

    Parser::ParserState Parser::analyzeHeaders() {
        if (headers.contains("Content-Length")) {
            contentLength = std::stoul(headers["Content-Length"]);
            transferEncoding = TransferEncoding::Identity;
            decoderQueue.emplace_back(new web::http::decoder::Identity(socketContext, contentLength));
        }
        if (headers.contains("Transfer-Encoding")) {
            const std::string& encoding = headers["Transfer-Encoding"];

            if (web::http::ciContains(encoding, "chunked")) {
                transferEncoding = TransferEncoding::Chunked;
                decoderQueue.emplace_back(new web::http::decoder::Chunked(socketContext));
            }
            if (web::http::ciContains(encoding, "compressed")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Compress(socketContext));
            }
            if (web::http::ciContains(encoding, "deflate")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Deflate(socketContext));
            }
            if (web::http::ciContains(encoding, "gzip")) {
                //  decoderQueue.emplace_back(new web::http::decoder::GZip(socketContext));
            }
        }
        if (decoderQueue.empty()) {
            transferEncoding = TransferEncoding::HTTP10;
            decoderQueue.emplace_back(new web::http::decoder::HTTP10Response(socketContext));
        }

        if (headers.contains("Content-Encoding")) {
            const std::string& encoding = headers["Content-Encoding"];

            if (web::http::ciContains(encoding, "compressed")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Compress(socketContext));
            }
            if (web::http::ciContains(encoding, "deflate")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Deflate(socketContext));
            }
            if (web::http::ciContains(encoding, "gzip")) {
                //  decoderQueue.emplace_back(new web::http::decoder::GZip(socketContext));
            }
            if (web::http::ciContains(encoding, "br")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Br(socketContext));
            }
        }

        return parseHeader();
    }

    std::size_t Parser::readContent() {
        ContentDecoder* contentDecoder = decoderQueue.front();

        const std::size_t consumed = contentDecoder->read();

        if (contentDecoder->isCompleted()) {
            contentDecoder = decoderQueue.back();

            std::vector<uint8_t> chunk = std::move(contentDecoder->getContent());

            content.insert(content.end(), chunk.begin(), chunk.end());

            parserState = parseContent(content);
        } else if (contentDecoder->isError()) {
            parserState = parsingError(400, "Wrong content encoding");
        }

        return consumed;
    }

    enum Parser::HTTPCompliance operator|(const enum Parser::HTTPCompliance& c1, const enum Parser::HTTPCompliance& c2) {
        return static_cast<enum Parser::HTTPCompliance>(static_cast<unsigned short>(c1) | static_cast<unsigned short>(c2));
    }

    enum Parser::HTTPCompliance operator&(const enum Parser::HTTPCompliance& c1, const enum Parser::HTTPCompliance& c2) {
        return static_cast<enum Parser::HTTPCompliance>(static_cast<unsigned short>(c1) & static_cast<unsigned short>(c2));
    }

} // namespace web::http
