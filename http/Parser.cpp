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

#include "http/Parser.h"

#include "http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cctype> // for isblank
#include <cstring>
#include <tuple> // for tie, tuple

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http {

    // HTTP/x.x
    std::regex Parser::httpVersionRegex("^HTTP/([[:digit:]])\\.([[:digit:]])$");

    void Parser::reset() {
        parserState = ParserState::BEGIN;
        headers.clear();
        contentLength = 0;
        if (content != nullptr) {
            delete[] content;
            content = nullptr;
        }
    }

    void Parser::parse(const char* junk, std::size_t junkLen) {
        std::size_t consumed = 0;

        while (consumed < junkLen && parserState != ParserState::ERROR) {
            switch (parserState) {
                case ParserState::BEGIN:
                    parserState = ParserState::FIRSTLINE;
                    begin();
                    [[fallthrough]];
                case ParserState::FIRSTLINE:
                    consumed += readStartLine(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::HEADER:
                    consumed += readHeaderLine(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::BODY:
                    consumed += readContent(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::ERROR:
                    break;
            };
        }
    }

    std::size_t Parser::readStartLine(const char* junk, std::size_t junkLen) {
        std::size_t consumed = 0;

        while (consumed < junkLen && parserState == ParserState::FIRSTLINE) {
            char ch = junk[consumed++];

            if (ch == '\r' || ch == '\n') {
                if (ch == '\n') {
                    parserState = parseStartLine(line);
                    line.clear();
                }
            } else {
                line += ch;
            }
        }

        return consumed;
    }

    std::size_t Parser::readHeaderLine(const char* junk, std::size_t junkLen) {
        std::size_t consumed = 0;

        while (consumed < junkLen && parserState == ParserState::HEADER) {
            char ch = junk[consumed];

            if (ch == '\r' || ch == '\n') {
                consumed++;
                if (ch == '\n') {
                    if (EOL) {
                        splitHeaderLine(line);
                        line.clear();
                        parserState = parseHeader();
                        EOL = false;
                    } else {
                        if (line.empty()) {
                            parserState = parseHeader();
                        } else {
                            EOL = true;
                        }
                    }
                }
            } else if (EOL) {
                if (std::isblank(ch)) {
                    if ((HTTPCompliance & HTTPCompliance::RFC7230) == HTTPCompliance::RFC7230) {
                        parserState = parsingError(400, "Header Folding");
                    } else {
                        line += ch;
                        consumed++;
                    }
                } else {
                    splitHeaderLine(line);
                    line.clear();
                }
                EOL = false;
            } else {
                line += ch;
                consumed++;
            }
        }

        return consumed;
    }

    void Parser::splitHeaderLine(const std::string& line) {
        if (!line.empty()) {
            std::string field;
            std::string value;
            std::tie(field, value) = httputils::str_split(line, ':');

            if (field.empty()) {
                parserState = parsingError(400, "Header-field empty");
            } else if (std::isblank(field.back()) || std::isblank(field.front())) {
                parserState = parsingError(400, "White space before or after header-field");
            } else if (value.empty()) {
                parserState = parsingError(400, "Header-value of field \"" + field + "\" empty");
            } else {
                httputils::to_lower(field);
                httputils::str_trimm(value);
                httputils::to_lower(value);

                if (headers.find(field) == headers.end()) {
                    headers.insert({field, value});
                } else {
                    headers[field] += "," + value;
                }
            }
        } else {
            parserState = parsingError(400, "Header-line empty");
        }
    }

    std::size_t Parser::readContent(const char* junk, std::size_t junkLen) {
        if (contentRead == 0) {
            content = new char[contentLength];
        }

        if (contentRead + junkLen <= contentLength) {
            memcpy(content + contentRead, junk, junkLen); // NOLINT(clang-analyzer-core.NonNullParamChecker)

            contentRead += junkLen;
            if (contentRead == contentLength) {
                parserState = parseContent(content, contentLength);

                delete[] content;
                content = nullptr;
                contentRead = 0;
            }
        } else {
            parserState = parsingError(400, "Content to long");

            if (content != nullptr) {
                delete[] content;
                content = nullptr;
            }
        }

        return junkLen;
    }

    enum Parser::HTTPCompliance operator|(const enum Parser::HTTPCompliance& c1, const enum Parser::HTTPCompliance& c2) {
        return static_cast<enum Parser::HTTPCompliance>(static_cast<unsigned short>(c1) | static_cast<unsigned short>(c2));
    }

    enum Parser::HTTPCompliance operator&(const enum Parser::HTTPCompliance& c1, const enum Parser::HTTPCompliance& c2) {
        return static_cast<enum Parser::HTTPCompliance>(static_cast<unsigned short>(c1) & static_cast<unsigned short>(c2));
    }

} // namespace http
