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

#ifndef WEB_HTTP_PARSER_H
#define WEB_HTTP_PARSER_H

#include "web/http/TransferEncoding.h" // IWYU pragma: export

namespace web::http {
    class ContentDecoder;
}

namespace core::socket::stream {
    class SocketContext;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/CiStringMap.h"

#include <cstddef>
#include <list> // IWYU pragma: export
#include <regex>
#include <string>
#include <vector>

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
        } hTTPCompliance;

    public:
        explicit Parser(core::socket::stream::SocketContext* socketContext,
                        const enum HTTPCompliance& compliance = HTTPCompliance::RFC2616 | HTTPCompliance::RFC7230);

        virtual ~Parser();

        std::size_t parse();

        void reset();

    protected:
        // Parser state
        enum struct ParserState { BEGIN, FIRSTLINE, HEADER, BODY, ERROR };

    private:
        ParserState parserState = ParserState::BEGIN;

    protected:
        TransferEncoding transferEncoding = TransferEncoding::HTTP10;

        static const std::regex httpVersionRegex;

    private:
        virtual void begin() = 0;
        virtual ParserState parseStartLine(const std::string& line) = 0;
        virtual ParserState parseHeader() = 0;
        virtual ParserState parseContent(std::vector<char>& content) = 0;
        virtual ParserState parseError(int code, const std::string& reason) = 0;

    protected:
        // Data common to all HTTP messages (Request/Response)
        std::size_t contentLength = 0;
        CiStringMap<std::string> headers;
        std::vector<char> content;

        int httpMajor = 0;
        int httpMinor = 0;

        std::list<web::http::ContentDecoder*> decoderQueue;

        core::socket::stream::SocketContext* socketContext = nullptr;

    private:
        std::size_t readStartLine();
        std::size_t readHeaderLine();
        void splitHeaderLine(const std::string& line);
        ParserState analyzeHeaders();
        std::size_t readContent();

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
