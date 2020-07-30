#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <regex>
#include <tuple>  // for tie, tuple
#include <vector> // for vector

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPRequestParser.h"
#include "Logger.h"
#include "httputils.h"

HTTPRequestParser::HTTPRequestParser(
    const std::function<void(std::string&, std::string&, std::string&)>& onRequest,
    const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, std::string>&)>& onHeader,
    const std::function<void(char*, size_t)>& onContent, const std::function<void(void)>& onParsed,
    const std::function<void(int status, const std::string& reason)>& onError)
    : onRequest(onRequest)
    , onHeader(onHeader)
    , onContent(onContent)
    , onParsed(onParsed)
    , onError(onError) {
}

HTTPRequestParser::HTTPRequestParser(
    const std::function<void(std::string&, std::string&, std::string&)>&& onRequest,
    const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, std::string>&)>&& onHeader,
    const std::function<void(char*, size_t)>&& onContent, const std::function<void(void)>&& onParsed,
    const std::function<void(int status, const std::string& reason)>&& onError)
    : onRequest(onRequest)
    , onHeader(onHeader)
    , onContent(onContent)
    , onParsed(onParsed)
    , onError(onError) {
}

void HTTPRequestParser::reset() {
    HTTPParser::reset();
    method.clear();
    originalUrl.clear();
    httpVersion.clear();
    cookies.clear();
    httpMajor = 0;
    httpMinor = 0;
}

// HTTP/x.x
static std::regex httpVersionRegex("^HTTP/([[:digit:]])\\.([[:digit:]])$");

enum HTTPParser::PAS HTTPRequestParser::parseStartLine(std::string& line) {
    enum HTTPParser::PAS PAS = HTTPParser::PAS::HEADER;

    if (!line.empty()) {
        std::string remaining;

        std::tie(method, remaining) = httputils::str_split(line, ' '); // if split not found second will be empty

        if (!methodSupported(method)) {
            PAS = parsingError(400, "Bad request method");
        } else if (remaining.empty()) {
            PAS = parsingError(400, "Malformed request");
        } else {
            std::tie(originalUrl, httpVersion) = httputils::str_split(remaining, ' ');

            originalUrl = httputils::url_decode(originalUrl);

            if (originalUrl.front() != '/') {
                PAS = parsingError(400, "Malformed URL");
            } else {
                std::smatch match;

                if (!std::regex_match(httpVersion, match, httpVersionRegex)) {
                    PAS = parsingError(400, "Wrong protocol-version");
                } else {
                    httpMajor = std::stoi(match.str(1));
                    httpMinor = std::stoi(match.str(2));

                    onRequest(method, originalUrl, httpVersion);
                    /*
                    std::string queries;
                    std::tie(std::ignore, queries) = httputils::str_split_last(originalUrl, '?');

                    while (!queries.empty()) {
                        std::string query;
                        std::tie(query, queries) = httputils::str_split(queries, '&');

                        onQuery(httputils::str_split(query, '='));
                        std::string key;
                        std::string value;
                        std::tie(key, value) = httputils::str_split(query, '=');

                        requestQueries[key] = value;

                    }
                    */
                }
            }
        }
    } else {
        PAS = parsingError(400, "Request-line empty");
    }

    return PAS;
}

enum HTTPParser::PAS HTTPRequestParser::parseHeader() {
    for (std::pair<std::string, std::string> headerField : header) {
        std::string& field = headerField.first;
        std::string& value = headerField.second;
        VLOG(1) << "++ Parse header field: " << field << " = " << value;
        if (field != "cookie") {
            if (field == "content-length") {
                contentLength = std::stoi(value);
            }
        } else {
            std::string cookiesLine = value;

            while (!cookiesLine.empty()) {
                std::string cookie;
                std::tie(cookie, cookiesLine) = httputils::str_split(cookiesLine, ';');

                std::string name;
                std::string value;
                std::tie(name, value) = httputils::str_split(cookie, '=');

                httputils::str_trimm(name);
                httputils::str_trimm(value);

                VLOG(1) << "++ Cookie: " << name << " = " << value;

                cookies.insert({name, value});
            }
        }
    }

    header.erase("cookie");

    onHeader(header, cookies);

    enum HTTPParser::PAS PAS = HTTPParser::PAS::BODY;

    if (contentLength == 0) {
        parsingFinished();
        PAS = PAS::FIRSTLINE;
    }

    return PAS;
}

enum HTTPParser::PAS HTTPRequestParser::parseContent(char* content, size_t size) {
    onContent(content, size);
    parsingFinished();

    return PAS::FIRSTLINE;
}

void HTTPRequestParser::parsingFinished() {
    onParsed();
    reset();
}

enum HTTPParser::PAS HTTPRequestParser::parsingError(int code, const std::string& reason) {
    onError(code, reason);
    reset();

    return PAS::ERROR;
}
