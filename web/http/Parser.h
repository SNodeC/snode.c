/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef WEB_HTTP_PARSER_H
#define WEB_HTTP_PARSER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for std::size_t
#include <map>
#include <regex>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    class Parser {
    protected:
        enum struct HTTPCompliance : unsigned short {
            RFC1945 = 0x01 << 0, // HTTP 1.0
            RFC2616 = 0x01 << 1, // HTTP 1.1
            RFC7230 = 0x01 << 2, // Message Syntax and Routing
            RFC7231 = 0x01 << 3, // Semantics and Content
            RFC7232 = 0x01 << 4, // Conditional Requests
            RFC7233 = 0x01 << 5, // Range Requests
            RFC7234 = 0x01 << 6, // Caching
            RFC7235 = 0x01 << 7, // Authentication
            RFC7540 = 0x01 << 8, // HTTP 2.0
            RFC7541 = 0x01 << 9  // Header Compression
        } HTTPCompliance;

    public:
        explicit Parser(enum HTTPCompliance compliance = HTTPCompliance::RFC2616 | HTTPCompliance::RFC7230)
            : HTTPCompliance(compliance) {
        }
        virtual ~Parser() = default;

        void parse(const char* junk, std::size_t junkLen);

    protected:
        // Parser state
        enum struct ParserState { BEGIN, FIRSTLINE, HEADER, BODY, ERROR } parserState = ParserState::BEGIN;
        static std::regex httpVersionRegex;

        virtual void reset();

        virtual void begin() = 0;
        virtual enum ParserState parseStartLine(const std::string& line) = 0;
        virtual enum ParserState parseHeader() = 0;
        virtual enum ParserState parseContent(char* content, std::size_t size) = 0;
        virtual enum ParserState parsingError(int code, const std::string& reason) = 0;

        // Data common to all HTTP messages (Request/Response)
        char* content = nullptr;
        std::size_t contentLength = 0;
        std::map<std::string, std::string> headers;

    private:
        std::size_t readStartLine(const char* junk, std::size_t junkLen);
        std::size_t readHeaderLine(const char* junk, std::size_t junkLen);
        void splitHeaderLine(const std::string& line);
        std::size_t readContent(const char* junk, std::size_t junkLen);

        // Line state
        bool EOL{false};

        // Used during parseing data
        std::string line;
        std::size_t contentRead = 0;

        friend enum HTTPCompliance operator|(const enum HTTPCompliance& c1, const enum HTTPCompliance& c2);
        friend enum HTTPCompliance operator&(const enum HTTPCompliance& c1, const enum HTTPCompliance& c2);
    };

} // namespace web::http

#endif // WEB_HTTP_PARSER_H
