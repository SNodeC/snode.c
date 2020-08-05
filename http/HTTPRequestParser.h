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

#ifndef HTTPREQUESTPARSER_H
#define HTTPREQUESTPARSER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <functional>
#include <map>
#include <set>
#include <string> // for string, basic_string, operator<

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPParser.h"

class HTTPRequestParser : public HTTPParser {
public:
    HTTPRequestParser(
        const std::function<void(std::string&, std::string&, std::string&, const std::map<std::string, std::string>&)>& onRequest,
        const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, std::string>&)>& onHeader,
        const std::function<void(char*, size_t)>& onContent, const std::function<void(void)>& onParsed,
        const std::function<void(int status, const std::string& reason)>& onError);

    HTTPRequestParser(
        const std::function<void(std::string&, std::string&, std::string&, const std::map<std::string, std::string>&)>&& onRequest,
        const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, std::string>&)>&& onHeader,
        const std::function<void(char*, size_t)>&& onContent, const std::function<void(void)>&& onParsed,
        const std::function<void(int status, const std::string& reason)>&& onError);

    void reset() override;

protected:
    // Check if request method is supported
    virtual bool methodSupported(const std::string& method) {
        return supportedMethods.contains(method);
    }

    // Parsers and Validators
    enum HTTPParser::PAS parseStartLine(std::string& line) override;
    enum HTTPParser::PAS parseHeader() override;
    enum HTTPParser::PAS parseContent(char* content, size_t size) override;

    // Exits
    void parsingFinished();
    enum HTTPParser::PAS parsingError(int code, const std::string& reason) override;

    // Supported http-methods
    std::set<std::string> supportedMethods{"GET", "PUT", "POST", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH", "HEAD"};

    // Data specific to HTTP request messages
    std::string method;
    std::string originalUrl;
    std::string httpVersion;
    std::map<std::string, std::string> cookies;
    std::map<std::string, std::string> queries;
    int httpMajor = 0;
    int httpMinor = 0;

    // Callbacks
    std::function<void(std::string&, std::string&, std::string&, const std::map<std::string, std::string>&)> onRequest;
    std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, std::string>&)> onHeader;
    std::function<void(char*, size_t)> onContent;
    std::function<void(void)> onParsed;
    std::function<void(int status, const std::string& reason)> onError;
};

#endif // HTTPREQUESTPARSER_H
