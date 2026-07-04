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
#include "web/http/server/RequestParser.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <functional>
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

    return testResult.processResult();
}
