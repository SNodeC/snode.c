#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <regex>
#include <tuple>  // for tie, tuple
#include <vector> // for vector

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPRequestParser.h"
#include "Logger.h"
#include "httputils.h"

HTTPRequestParser::HTTPRequestParser(const std::function<void(std::string&, std::string&, std::string&)>& onRequest,
                                     const std::function<void(const std::string&, const std::string&)>& onHeader,
                                     const std::function<void(const std::string&, const std::string&)>& onCookie,
                                     const std::function<void(char* body, size_t bodyLength)>& onBody,
                                     const std::function<void(void)>& onParsed,
                                     const std::function<void(int status, const std::string& reason)>& onError)
    : onRequest(onRequest)
    , onHeader(onHeader)
    , onCookie(onCookie)
    , onBody(onBody)
    , onParsed(onParsed)
    , onError(onError) {
}

HTTPRequestParser::HTTPRequestParser(const std::function<void(std::string&, std::string&, std::string&)>&& onRequest,
                                     const std::function<void(const std::string&, const std::string&)>&& onHeader,
                                     const std::function<void(const std::string&, const std::string&)>&& onCookie,
                                     const std::function<void(char* body, size_t bodyLength)>&& onBody,
                                     const std::function<void(void)>&& onParsed,
                                     const std::function<void(int status, const std::string& reason)>&& onError)
    : onRequest(onRequest)
    , onHeader(onHeader)
    , onCookie(onCookie)
    , onBody(onBody)
    , onParsed(onParsed)
    , onError(onError) {
}

void HTTPRequestParser::reset() {
    HTTPParser::reset();
    method.clear();
    originalUrl.clear();
    httpVersion.clear();
    header.clear();
    httpMajor = 0;
    httpMinor = 0;
}

// HTTP/x.x
static std::regex httpVersionRegex("^HTTP/([[:digit:]])\\.([[:digit:]])$");

void HTTPRequestParser::parseStartLine(std::string& line) {
    if (!line.empty()) {
        std::string remaining;
        std::tie(method, remaining) = httputils::str_split(line, ' '); // if split not found second will be empty

        if (!methodSupported(method)) {
            parsingError(400, "Bad request method");
        } else if (remaining.empty()) {
            parsingError(400, "Malformed request");
        } else {
            std::tie(originalUrl, httpVersion) = httputils::str_split(remaining, ' ');

            originalUrl = httputils::url_decode(originalUrl);

            if (originalUrl.front() != '/') {
                parsingError(400, "Malformed URL");
            } else {
                std::smatch match;

                if (!std::regex_match(httpVersion, match, httpVersionRegex)) {
                    parsingError(400, "Wrong protocol-version");
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
        parsingError(400, "Request-line empty");
        PAS = PAS::ERROR;
    }
}

void HTTPRequestParser::parseHeaderLine(const std::string& field, const std::string& value) {
    VLOG(1) << "++ Header or Cookie: " << field << " = " << value;
    header.insert({field, value});
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
            onHeader(field, value);
        } else {
            std::string cookies = value;

            while (!cookies.empty()) {
                std::string cookie;
                std::tie(cookie, cookies) = httputils::str_split(cookies, ';');

                std::string name;
                std::string value;
                std::tie(name, value) = httputils::str_split(cookie, '=');

                httputils::str_trimm(name);
                httputils::str_trimm(value);

                onCookie(name, value);
            }
        }
    }

    return (contentLength > 0) ? PAS::BODY : PAS::COMPLETE;
}

void HTTPRequestParser::parseBodyData(char* body, size_t size) {
    onBody(body, size);
}

void HTTPRequestParser::parsingFinished() {
    onParsed();
    reset();
}

void HTTPRequestParser::parsingError(int code, const std::string& reason) {
    PAS = PAS::ERROR;
    onError(code, reason);
    reset();
}
