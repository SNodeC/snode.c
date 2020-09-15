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

#ifndef PARSER_H
#define PARSER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <map>
#include <regex>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http {

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
        Parser(enum HTTPCompliance compliance = HTTPCompliance::RFC2616 | HTTPCompliance::RFC7230)
            : HTTPCompliance(compliance) {
        }
        virtual ~Parser() = default;

        void parse(const char* buf, size_t count);

    protected:
        // Parser state
        enum struct [[nodiscard]] PAS{FIRSTLINE, HEADER, BODY, ERROR} PAS = PAS::FIRSTLINE;
        static std::regex httpVersionRegex;

        virtual void reset();

        virtual enum PAS parseStartLine(std::string& line) = 0;
        virtual enum PAS parseHeader() = 0;
        virtual enum PAS parseContent(char* content, size_t size) = 0;
        virtual enum PAS parsingError(int code, const std::string& reason) = 0;

        // Data common to all HTTP messages (Request/Response)
        char* content = nullptr;
        size_t contentLength = 0;
        std::map<std::string, std::string> headers;

    private:
        size_t readStartLine(const char* buf, size_t count);
        size_t readHeaderLine(const char* buf, size_t count);
        void splitHeaderLine(const std::string& line);
        size_t readContent(const char* buf, size_t count);

        // Line state
        bool EOL{false};

        // Used during parseing data
        std::string line;
        size_t contentRead = 0;

        friend enum HTTPCompliance operator|(const enum HTTPCompliance& c1, const enum HTTPCompliance& c2);
        friend enum HTTPCompliance operator&(const enum HTTPCompliance& c1, const enum HTTPCompliance& c2);
    };

} // namespace http

#endif // PARSER_H
