/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef WEB_HTTP_PARSER_H
#define WEB_HTTP_PARSER_H

#include "web/http/TransferEncoding.h" // IWYU pragma: export
#include "web/http/decoder/Fields.h"

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
#include <set>
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
        enum struct ParserState { BEGIN, FIRSTLINE, HEADER, BODY, TRAILER, ERROR };

        ParserState parserState = ParserState::BEGIN;

        TransferEncoding transferEncoding = TransferEncoding::HTTP10;

        static const std::regex httpVersionRegex;

    private:
        virtual void begin() = 0;
        virtual void parseStartLine(const std::string& line) = 0;

    protected:
        virtual void analyzeHeader();

    private:
        virtual void parseError(int code, const std::string& reason) = 0;
        virtual void parsingFinished() = 0;

    protected:
        // Data common to all HTTP messages (Request/Response)
        CiStringMap<std::string> headers;
        std::vector<char> content;

        int httpMajor = 0;
        int httpMinor = 0;

        std::list<web::http::ContentDecoder*> decoderQueue;

        core::socket::stream::SocketContext* socketContext = nullptr;

    private:
        web::http::decoder::Fields headerDecoder;

        std::set<std::string> trailerFieldsExpected;
        web::http::decoder::Fields trailerDecoder;

        std::size_t readStartLine();
        std::size_t readHeader();
        std::size_t readContent();
        std::size_t readTrailer();

    protected:
        // Used during parseing data
        std::string line;
        std::size_t contentLength = 0;
        std::size_t contentLengthRead = 0;

        friend enum HTTPCompliance operator|(const enum HTTPCompliance& c1, const enum HTTPCompliance& c2);
        friend enum HTTPCompliance operator&(const enum HTTPCompliance& c1, const enum HTTPCompliance& c2);
    };

} // namespace web::http

#endif // WEB_HTTP_PARSER_H
