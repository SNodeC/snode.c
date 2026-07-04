/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "support/TestResult.h"
#include "web/http/CiStringMap.h"
#include "web/http/client/ResponseParser.h"
#include "web/http/http_utils.h"
#include "web/http/server/RequestParser.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    class DummySocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "dummy";
        }
    };

    class BufferSocketConnection : public core::socket::stream::SocketConnection {
    public:
        explicit BufferSocketConnection(std::string input)
            : SocketConnection(-1, "HttpMessageParserTest", nullptr)
            , input(std::move(input)) {
        }

        int getFd() const override {
            return -1;
        }

        void sendToPeer(const char*, std::size_t) override {
        }

        bool streamToPeer(core::pipe::Source*) override {
            return false;
        }

        void streamEof() override {
        }

        std::size_t readFromPeer(char* chunk, std::size_t chunkLen) override {
            const std::size_t available = input.size() - offset;
            const std::size_t toRead = std::min(chunkLen, available);

            if (toRead > 0) {
                std::memcpy(chunk, input.data() + offset, toRead);
                offset += toRead;
            }

            return toRead;
        }

        void shutdownRead() override {
        }

        void shutdownWrite() override {
        }

        const core::socket::SocketAddress& getBindAddress() const override {
            return address;
        }

        const core::socket::SocketAddress& getLocalAddress() const override {
            return address;
        }

        const core::socket::SocketAddress& getRemoteAddress() const override {
            return address;
        }

        void close() override {
        }

        void setTimeout(const utils::Timeval&) override {
        }

        void setReadTimeout(const utils::Timeval&) override {
        }

        void setWriteTimeout(const utils::Timeval&) override {
        }

        std::size_t getTotalSent() const override {
            return 0;
        }

        std::size_t getTotalQueued() const override {
            return 0;
        }

        std::size_t getTotalRead() const override {
            return offset;
        }

        std::size_t getTotalProcessed() const override {
            return offset;
        }

    private:
        std::string input;
        std::size_t offset = 0;
        DummySocketAddress address;
    };

    class BufferSocketContext : public core::socket::stream::SocketContext {
    public:
        explicit BufferSocketContext(core::socket::stream::SocketConnection* socketConnection)
            : SocketContext(socketConnection) {
        }

    private:
        void onConnected() override {
        }

        void onDisconnected() override {
        }

        std::size_t onReceivedFromPeer() override {
            return 0;
        }

        bool onSignal(int) override {
            return false;
        }
    };

    std::string bodyToString(const std::vector<char>& body) {
        return std::string(body.begin(), body.end());
    }

    struct RequestParseResult {
        bool started = false;
        bool parsed = false;
        int errorCode = 0;
        std::string errorReason;
        std::optional<web::http::server::Request> request;
        std::size_t consumed = 0;
    };

    struct ResponseParseResult {
        bool started = false;
        bool parsed = false;
        int errorCode = 0;
        std::string errorReason;
        web::http::client::Response* response = nullptr;
        std::size_t consumed = 0;
    };

    RequestParseResult parseRequestMessage(const std::string& message) {
        BufferSocketConnection connection(message);
        BufferSocketContext context(&connection);
        RequestParseResult result;

        web::http::server::RequestParser parser(
            &context,
            [&result]() {
                result.started = true;
            },
            [&result](web::http::server::Request&& request) {
                result.parsed = true;
                result.request.emplace(std::move(request));
            },
            [&result](int code, const std::string& reason) {
                result.errorCode = code;
                result.errorReason = reason;
            });

        result.consumed = parser.parse();

        return result;
    }

    ResponseParseResult parseResponseMessage(const std::string& message) {
        BufferSocketConnection connection(message);
        BufferSocketContext context(&connection);
        ResponseParseResult result;

        web::http::client::ResponseParser parser(
            &context,
            [&result]() {
                result.started = true;
            },
            [&result](web::http::client::Response& response) {
                result.parsed = true;
                result.response = &response;
            },
            [&result](int code, const std::string& reason) {
                result.errorCode = code;
                result.errorReason = reason;
            });

        result.consumed = parser.parse();

        return result;
    }

} // namespace

int main() {
    tests::support::TestResult testResult;

    {
        web::http::CiStringMap<std::string> headers;
        headers.emplace("Content-Type", "text/plain");
        testResult.expectTrue(headers.find("content-type") != headers.end(), "HTTP header map lookup is case-insensitive");
        testResult.expectTrue(headers["CONTENT-TYPE"] == "text/plain", "HTTP header map retrieves values case-insensitively");
        testResult.expectTrue(headers.find("Missing") == headers.end(), "HTTP header map reports missing headers");
    }

    {
        BufferSocketConnection connection("POST /items/search?q=snode%2Ec&empty= HTTP/1.1\r\nHost: example.test\r\nX-Test: one\r\nx-test: two\r\nContent-Length: 7\r\n\r\npayload");
        BufferSocketContext context(&connection);
        bool started = false;
        bool parsed = false;
        int errorCode = 0;
        std::string errorReason;
        std::optional<web::http::server::Request> parsedRequest;

        web::http::server::RequestParser parser(
            &context,
            [&started]() { started = true; },
            [&parsed, &parsedRequest](web::http::server::Request&& request) {
                parsed = true;
                parsedRequest.emplace(std::move(request));
            },
            [&errorCode, &errorReason](int code, const std::string& reason) {
                errorCode = code;
                errorReason = reason;
            });

        parser.parse();

        testResult.expectTrue(started, "request parser emits start callback");
        testResult.expectTrue(parsed, "request parser emits parsed callback");
        testResult.expectEqual(0, errorCode, "request parser does not emit an error");
        testResult.expectTrue(errorReason.empty(), "request parser leaves error reason empty on success");
        testResult.expectTrue(parsedRequest->method == "POST", "request parser captures method");
        testResult.expectTrue(parsedRequest->url == "/items/search?q=snode%2Ec&empty=", "request parser captures request target");
        testResult.expectTrue(parsedRequest->httpVersion == "HTTP/1.1", "request parser captures HTTP version token");
        testResult.expectEqual(1, parsedRequest->httpMajor, "request parser captures HTTP major version");
        testResult.expectEqual(1, parsedRequest->httpMinor, "request parser captures HTTP minor version");
        testResult.expectTrue(parsedRequest->query("q") == "snode.c", "request parser decodes exposed query values");
        testResult.expectTrue(parsedRequest->query("empty").empty(), "request parser exposes empty query values as empty strings");
        testResult.expectTrue(parsedRequest->get("host") == "example.test", "request parser exposes headers case-insensitively");
        testResult.expectTrue(parsedRequest->get("X-Test") == "one,two", "request parser combines repeated headers with a comma");
        testResult.expectTrue(parsedRequest->get("missing").empty(), "request parser returns an empty string for missing headers");
        testResult.expectTrue(bodyToString(parsedRequest->body) == "payload", "request parser captures Content-Length body");
    }

    {
        BufferSocketConnection connection("HTTP/1.1 201 Created\r\nContent-Type: application/json\r\nSet-Cookie: sid=abc; Path=/\r\nContent-Length: 2\r\n\r\n{}");
        BufferSocketContext context(&connection);
        bool started = false;
        bool parsed = false;
        int errorCode = 0;
        std::string errorReason;
        web::http::client::Response* parsedResponse = nullptr;

        web::http::client::ResponseParser parser(
            &context,
            [&started]() { started = true; },
            [&parsed, &parsedResponse](web::http::client::Response& response) {
                parsed = true;
                parsedResponse = &response;
            },
            [&errorCode, &errorReason](int code, const std::string& reason) {
                errorCode = code;
                errorReason = reason;
            });

        parser.parse();

        testResult.expectTrue(started, "response parser emits start callback");
        testResult.expectTrue(parsed, "response parser emits parsed callback");
        testResult.expectEqual(0, errorCode, "response parser does not emit an error");
        testResult.expectTrue(errorReason.empty(), "response parser leaves error reason empty on success");
        testResult.expectTrue(parsedResponse != nullptr, "response parser provides a response object");
        if (parsedResponse != nullptr) {
            testResult.expectTrue(parsedResponse->httpVersion == "HTTP/1.1", "response parser captures HTTP version token");
            testResult.expectEqual(1, parsedResponse->httpMajor, "response parser captures HTTP major version");
            testResult.expectEqual(1, parsedResponse->httpMinor, "response parser captures HTTP minor version");
            testResult.expectTrue(parsedResponse->statusCode == "201", "response parser captures status code");
            testResult.expectTrue(parsedResponse->reason == "Created", "response parser captures reason phrase");
            testResult.expectTrue(parsedResponse->get("content-type") == "application/json", "response parser exposes headers case-insensitively");
            testResult.expectTrue(parsedResponse->get("set-cookie").empty(), "response parser removes Set-Cookie from plain headers");
            testResult.expectTrue(parsedResponse->cookie("sid") == "abc", "response parser exposes Set-Cookie values through cookie lookup");
            testResult.expectTrue(parsedResponse->get("missing").empty(), "response parser returns an empty string for missing headers");
            testResult.expectTrue(bodyToString(parsedResponse->body) == "{}", "response parser captures Content-Length body");
        }
    }

    {
        const RequestParseResult malformedLine = parseRequestMessage("GET example.test HTTP/1.1\r\n\r\n");
        testResult.expectTrue(malformedLine.started, "request parser starts before rejecting malformed request target");
        testResult.expectTrue(!malformedLine.parsed, "request parser does not parse malformed request target");
        testResult.expectEqual(400, malformedLine.errorCode, "request parser reports malformed request target as bad request");
        testResult.expectTrue(!malformedLine.errorReason.empty(), "request parser supplies an error reason for malformed request target");

        const RequestParseResult unsupportedVersion = parseRequestMessage("GET / HTTP/2.0\r\n\r\n");
        testResult.expectTrue(unsupportedVersion.started, "request parser starts before rejecting unsupported HTTP version");
        testResult.expectTrue(!unsupportedVersion.parsed, "request parser does not parse unsupported HTTP version");
        testResult.expectEqual(505, unsupportedVersion.errorCode, "request parser reports unsupported HTTP version");
        testResult.expectTrue(!unsupportedVersion.errorReason.empty(),
                              "request parser supplies an error reason for unsupported HTTP version");

        const RequestParseResult malformedHeader = parseRequestMessage("GET / HTTP/1.1\r\n : value\r\n\r\n");
        testResult.expectTrue(malformedHeader.started, "request parser starts before rejecting malformed header whitespace");
        testResult.expectTrue(!malformedHeader.parsed, "request parser does not parse malformed header whitespace");
        testResult.expectEqual(400, malformedHeader.errorCode, "request parser reports malformed header whitespace as bad request");
        testResult.expectTrue(!malformedHeader.errorReason.empty(),
                              "request parser supplies an error reason for malformed header whitespace");

        const RequestParseResult invalidContentLength = parseRequestMessage("POST / HTTP/1.1\r\nContent-Length: nope\r\n\r\n");
        testResult.expectTrue(invalidContentLength.started, "request parser starts before rejecting invalid Content-Length");
        testResult.expectTrue(!invalidContentLength.parsed, "request parser does not parse invalid Content-Length");
        testResult.expectEqual(400, invalidContentLength.errorCode, "request parser reports invalid Content-Length as bad request");
        testResult.expectTrue(!invalidContentLength.errorReason.empty(),
                              "request parser supplies an error reason for invalid Content-Length");

        const RequestParseResult incompleteBody = parseRequestMessage("POST / HTTP/1.1\r\nContent-Length: 7\r\n\r\nabc");
        testResult.expectTrue(incompleteBody.started, "request parser starts incomplete body message");
        testResult.expectTrue(!incompleteBody.parsed, "request parser leaves incomplete body unparsed without EOF error semantics");
        testResult.expectEqual(
            0, incompleteBody.errorCode, "request parser does not invent an error for incomplete body at buffer boundary");
        testResult.expectTrue(incompleteBody.errorReason.empty(), "request parser leaves error reason empty for incomplete body boundary");

        const RequestParseResult emptyInput = parseRequestMessage("");
        testResult.expectTrue(emptyInput.started, "request parser start callback runs for empty input parse attempt");
        testResult.expectTrue(!emptyInput.parsed, "request parser does not parse empty input");
        testResult.expectEqual(0, emptyInput.errorCode, "request parser does not invent an error for empty input boundary");
        testResult.expectTrue(emptyInput.errorReason.empty(), "request parser leaves error reason empty for empty input boundary");
    }

    {
        const ResponseParseResult malformedLine = parseResponseMessage("NOTHTTP 200 OK\r\n\r\n");
        testResult.expectTrue(malformedLine.started, "response parser starts before rejecting malformed status line");
        testResult.expectTrue(!malformedLine.parsed, "response parser does not parse malformed status line");
        testResult.expectEqual(400, malformedLine.errorCode, "response parser reports malformed status line as bad response");
        testResult.expectTrue(!malformedLine.errorReason.empty(), "response parser supplies an error reason for malformed status line");

        const ResponseParseResult unsupportedVersion = parseResponseMessage("HTTP/2.0 200 OK\r\n\r\n");
        testResult.expectTrue(unsupportedVersion.started, "response parser starts before rejecting unsupported HTTP version");
        testResult.expectTrue(!unsupportedVersion.parsed, "response parser does not parse unsupported HTTP version");
        testResult.expectEqual(400, unsupportedVersion.errorCode, "response parser reports unsupported HTTP version as bad response");
        testResult.expectTrue(!unsupportedVersion.errorReason.empty(),
                              "response parser supplies an error reason for unsupported HTTP version");

        const ResponseParseResult unknownStatusCode = parseResponseMessage("HTTP/1.1 999 Unknown\r\n\r\n");
        testResult.expectTrue(unknownStatusCode.started, "response parser starts before rejecting unknown status code");
        testResult.expectTrue(!unknownStatusCode.parsed, "response parser does not parse unknown status code");
        testResult.expectEqual(400, unknownStatusCode.errorCode, "response parser reports unknown status code as bad response");
        testResult.expectTrue(!unknownStatusCode.errorReason.empty(), "response parser supplies an error reason for unknown status code");

        const ResponseParseResult malformedHeader = parseResponseMessage("HTTP/1.1 200 OK\r\n : value\r\n\r\n");
        testResult.expectTrue(malformedHeader.started, "response parser starts before rejecting malformed header whitespace");
        testResult.expectTrue(!malformedHeader.parsed, "response parser does not parse malformed header whitespace");
        testResult.expectEqual(400, malformedHeader.errorCode, "response parser reports malformed header whitespace as bad response");
        testResult.expectTrue(!malformedHeader.errorReason.empty(),
                              "response parser supplies an error reason for malformed header whitespace");

        const ResponseParseResult invalidContentLength = parseResponseMessage("HTTP/1.1 200 OK\r\nContent-Length: nope\r\n\r\n");
        testResult.expectTrue(invalidContentLength.started, "response parser starts before rejecting invalid Content-Length");
        testResult.expectTrue(!invalidContentLength.parsed, "response parser does not parse invalid Content-Length");
        testResult.expectEqual(400, invalidContentLength.errorCode, "response parser reports invalid Content-Length as bad response");
        testResult.expectTrue(!invalidContentLength.errorReason.empty(),
                              "response parser supplies an error reason for invalid Content-Length");

        const ResponseParseResult incompleteBody = parseResponseMessage("HTTP/1.1 200 OK\r\nContent-Length: 7\r\n\r\nabc");
        testResult.expectTrue(incompleteBody.started, "response parser starts incomplete body message");
        testResult.expectTrue(!incompleteBody.parsed, "response parser leaves incomplete body unparsed without EOF error semantics");
        testResult.expectEqual(
            0, incompleteBody.errorCode, "response parser does not invent an error for incomplete body at buffer boundary");
        testResult.expectTrue(incompleteBody.errorReason.empty(), "response parser leaves error reason empty for incomplete body boundary");

        const ResponseParseResult emptyInput = parseResponseMessage("");
        testResult.expectTrue(emptyInput.started, "response parser start callback runs for empty input parse attempt");
        testResult.expectTrue(!emptyInput.parsed, "response parser does not parse empty input");
        testResult.expectEqual(0, emptyInput.errorCode, "response parser does not invent an error for empty input boundary");
        testResult.expectTrue(emptyInput.errorReason.empty(), "response parser leaves error reason empty for empty input boundary");
    }

    {
        web::http::CiStringMap<std::string> headers;
        headers.emplace("Content-Length", "5");
        headers.emplace("Content-Type", "text/plain");
        web::http::CiStringMap<std::string> trailers;
        trailers.emplace("Expires", "never");
        web::http::CiStringMap<std::string> cookies;
        cookies.emplace("sid", "abc");
        const std::vector<char> body{'h', 'e', 'l', 'l', 'o'};

        const std::string formattedRequest =
            httputils::toString("POST", "/submit", "HTTP/1.1", {{"q", "snode.c"}}, headers, trailers, cookies, body);
        testResult.expectTrue(formattedRequest.find("Request") != std::string::npos, "request formatter includes request section label");
        testResult.expectTrue(formattedRequest.find("Method") != std::string::npos && formattedRequest.find("POST") != std::string::npos,
                              "request formatter includes method");
        testResult.expectTrue(formattedRequest.find("Url") != std::string::npos && formattedRequest.find("/submit") != std::string::npos,
                              "request formatter includes URL");
        testResult.expectTrue(formattedRequest.find("Version") != std::string::npos &&
                                  formattedRequest.find("HTTP/1.1") != std::string::npos,
                              "request formatter includes HTTP version");
        testResult.expectTrue(formattedRequest.find("Content-Length") != std::string::npos &&
                                  formattedRequest.find("5") != std::string::npos,
                              "request formatter includes headers");
        testResult.expectTrue(formattedRequest.find("sid") != std::string::npos && formattedRequest.find("abc") != std::string::npos,
                              "request formatter includes cookies");
        testResult.expectTrue(formattedRequest.find("Body") != std::string::npos, "request formatter includes body section");

        web::http::CiStringMap<web::http::CookieOptions> responseCookies;
        responseCookies.emplace("sid", web::http::CookieOptions("abc", {{"Path", "/"}}));
        const std::string formattedResponse = httputils::toString("HTTP/1.1", "201", "Created", headers, responseCookies, body);
        testResult.expectTrue(formattedResponse.find("Response") != std::string::npos,
                              "response formatter includes response section label");
        testResult.expectTrue(formattedResponse.find("Version") != std::string::npos &&
                                  formattedResponse.find("HTTP/1.1") != std::string::npos,
                              "response formatter includes HTTP version");
        testResult.expectTrue(formattedResponse.find("Status") != std::string::npos && formattedResponse.find("201") != std::string::npos,
                              "response formatter includes status code");
        testResult.expectTrue(formattedResponse.find("Reason") != std::string::npos &&
                                  formattedResponse.find("Created") != std::string::npos,
                              "response formatter includes reason phrase");
        testResult.expectTrue(formattedResponse.find("Content-Type") != std::string::npos &&
                                  formattedResponse.find("text/plain") != std::string::npos,
                              "response formatter includes headers");
        testResult.expectTrue(formattedResponse.find("Path=/") != std::string::npos, "response formatter includes cookie options");
        testResult.expectTrue(formattedResponse.find("Body") != std::string::npos, "response formatter includes body section");
    }

    return testResult.processResult();
}
