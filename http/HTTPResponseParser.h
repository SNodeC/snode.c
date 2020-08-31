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

#ifndef HTTPRESPONSEPARSER_H
#define HTTPRESPONSEPARSER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPParser.h"

namespace http {

    class ResponseCookie {
    public:
        ResponseCookie(const std::string& name, const std::string& value)
            : name(name)
            , value(value) {
        }

        void setOption(const std::string& name, const std::string& value) {
            options[name] = value;
        }

        std::map<std::string, std::string> getOptions() {
            return options;
        }

        std::string& getValue() {
            return value;
        }

    protected:
        std::string name;
        std::string value;
        std::map<std::string, std::string> options;

        friend class HTTPResponseParser;
    };

    class HTTPResponseParser : public HTTPParser {
    public:
        HTTPResponseParser(
            const std::function<void(const std::string&, const std::string&, const std::string&)>& onResponse,
            const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, ResponseCookie>&)>& onHeader,
            const std::function<void(char*, size_t)>& onContent, const std::function<void(HTTPResponseParser&)>& onParsed,
            const std::function<void(int status, const std::string& reason)>& onError);

        HTTPResponseParser(
            const std::function<void(const std::string&, const std::string&, const std::string&)>&& onResponse,
            const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, ResponseCookie>&)>&& onHeader,
            const std::function<void(char*, size_t)>&& onContent, const std::function<void(HTTPResponseParser&)>&& onParsed,
            const std::function<void(int status, const std::string& reason)>&& onError);

        enum HTTPParser::PAS parseStartLine(std::string& line) override;
        enum HTTPParser::PAS parseHeader() override;
        enum HTTPParser::PAS parseContent(char* content, size_t size) override;
        enum HTTPParser::PAS parsingError(int code, const std::string& reason) override;

        void reset() override;

    protected:
        void parsingFinished();

        std::string httpVersion;
        std::string statusCode;
        std::string reason;
        std::map<std::string, ResponseCookie> cookies;

        std::function<void(const std::string&, const std::string&, const std::string&)> onResponse;
        std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, ResponseCookie>&)> onHeader;
        std::function<void(char*, size_t)> onContent;
        std::function<void(HTTPResponseParser&)> onParsed;
        std::function<void(int status, const std::string& reason)> onError;
    };

} // namespace http

#endif // HTTPRESPONSEPARSER_H
