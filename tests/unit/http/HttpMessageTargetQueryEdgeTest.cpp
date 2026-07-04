/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "support/TestResult.h"
#include "web/http/server/RequestParser.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <map>
#include <string>
#include <utility>

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
            : SocketConnection(-1, "HttpMessageTargetQueryEdgeTest", nullptr)
            , input(std::move(input)) {
        }

        int getFd() const override { return -1; }
        void sendToPeer(const char*, std::size_t) override {}
        bool streamToPeer(core::pipe::Source*) override { return false; }
        void streamEof() override {}

        std::size_t readFromPeer(char* chunk, std::size_t chunkLen) override {
            const std::size_t available = input.size() - offset;
            const std::size_t toRead = std::min(chunkLen, available);
            if (toRead > 0) {
                std::memcpy(chunk, input.data() + offset, toRead);
                offset += toRead;
            }
            return toRead;
        }

        void shutdownRead() override {}
        void shutdownWrite() override {}
        const core::socket::SocketAddress& getBindAddress() const override { return address; }
        const core::socket::SocketAddress& getLocalAddress() const override { return address; }
        const core::socket::SocketAddress& getRemoteAddress() const override { return address; }
        void close() override {}
        void setTimeout(const utils::Timeval&) override {}
        void setReadTimeout(const utils::Timeval&) override {}
        void setWriteTimeout(const utils::Timeval&) override {}
        std::size_t getTotalSent() const override { return 0; }
        std::size_t getTotalQueued() const override { return 0; }
        std::size_t getTotalRead() const override { return offset; }
        std::size_t getTotalProcessed() const override { return offset; }

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
        void onConnected() override {}
        void onDisconnected() override {}
        std::size_t onReceivedFromPeer() override { return 0; }
        bool onSignal(int) override { return false; }
    };

    struct ParsedTarget {
        std::string url;
        std::map<std::string, std::string> queries;

        std::string query(const std::string& key) const {
            const auto it = queries.find(key);
            return it != queries.end() ? it->second : std::string();
        }
    };

    ParsedTarget parseRequestTarget(const std::string& target) {
        BufferSocketConnection connection("GET " + target + " HTTP/1.1\r\nHost: example.test\r\n\r\n");
        BufferSocketContext context(&connection);
        ParsedTarget parsedTarget;

        web::http::server::RequestParser parser(
            &context,
            []() {},
            [&parsedTarget](web::http::server::Request&& request) {
                parsedTarget.url = request.url;
                parsedTarget.queries = std::move(request.queries);
            },
            [](int, const std::string&) {});

        parser.parse();
        return parsedTarget;
    }

} // namespace

int main() {
    tests::support::TestResult testResult;

    {
        const ParsedTarget request = parseRequestTarget("/plain");
        testResult.expectTrue(request.url == "/plain", "parser preserves a target without a query string");
        testResult.expectTrue(request.queries.empty(), "parser leaves query map empty when no query string is present");
    }

    {
        const ParsedTarget request = parseRequestTarget("/search?q=snodec");
        testResult.expectTrue(request.url == "/search?q=snodec", "parser preserves a target with one query parameter");
        testResult.expectTrue(request.query("q") == "snodec", "parser exposes a single query parameter value");
    }

    {
        const ParsedTarget request = parseRequestTarget("/search?q=snodec&limit=10");
        testResult.expectTrue(request.query("q") == "snodec", "parser exposes the first query parameter among multiple parameters");
        testResult.expectTrue(request.query("limit") == "10", "parser exposes the second query parameter among multiple parameters");
    }

    {
        const ParsedTarget request = parseRequestTarget("/search?q=");
        testResult.expectTrue(request.query("q").empty(), "parser exposes empty query values as empty strings");
    }

    {
        const ParsedTarget request = parseRequestTarget("/search?q=one&q=two");
        testResult.expectTrue(request.query("q") == "two", "parser keeps the last value for repeated query keys");
    }

    {
        const ParsedTarget request = parseRequestTarget("/search?q=snode%20c");
        testResult.expectTrue(request.query("q") == "snode c", "parser decodes percent-encoded query values at the message layer");
    }

    return testResult.processResult();
}
