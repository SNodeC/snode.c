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
#include "web/http/ContentDecoder.h"

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

    // HTTP/x.x
    const std::regex Parser::httpVersionRegex("^HTTP/([[:digit:]])\\.([[:digit:]])$");

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

        decoderQueue.resize(0);
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
                if (ch == '\r' || ch == '\n') {
                    consumed++;
                    if (ch == '\n') {
                        if (EOL) {
                            splitHeaderLine(line);
                            line.clear();
                            if (parserState != ParserState::ERROR) {
                                parserState = parseHeader();
                            }
                            EOL = false;
                        } else if (line.empty()) {
                            if (parserState != ParserState::ERROR) {
                                parserState = parseHeader();
                            }
                        } else {
                            EOL = true;
                        }
                    }
                } else if (EOL) {
                    if (std::isblank(ch) != 0) {
                        if ((hTTPCompliance & HTTPCompliance::RFC7230) == HTTPCompliance::RFC7230) {
                            parserState = parsingError(400, "Header Folding");
                        } else {
                            line += ch;
                            consumed++;
                        }
                    } else {
                        splitHeaderLine(line);
                        line.clear();
                        line += ch;
                        consumed++;
                    }
                    EOL = false;
                } else {
                    line += ch;
                    consumed++;
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

    std::size_t Parser::readContent1() {
        static char contentJunk[MAX_CONTENT_JUNK_LEN];

        std::size_t consumed = 0;

        if (httpMinor == 0 && contentLength == 0) {
            consumed = socketContext->readFromPeer(contentJunk, MAX_CONTENT_JUNK_LEN);

            const std::size_t contentJunkLen = static_cast<std::size_t>(consumed);
            if (contentJunkLen > 0) {
                content.insert(content.end(), contentJunk, contentJunk + contentJunkLen);
            } else {
                parserState = parseContent(content);
            }
        } else if (httpMinor == 1) {
            const std::size_t contentJunkLenLeft =
                (contentLength - contentRead < MAX_CONTENT_JUNK_LEN) ? contentLength - contentRead : MAX_CONTENT_JUNK_LEN;

            consumed = socketContext->readFromPeer(contentJunk, contentJunkLenLeft);

            const std::size_t contentJunkLen = static_cast<std::size_t>(consumed);
            if (contentJunkLen > 0) {
                if (contentRead + contentJunkLen <= contentLength) {
                    content.insert(content.end(), contentJunk, contentJunk + contentJunkLen);

                    contentRead += contentJunkLen;
                    if (contentRead == contentLength) {
                        parserState = parseContent(content);
                    }
                } else {
                    parserState = parsingError(400, "Content to long");
                }
            }
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
