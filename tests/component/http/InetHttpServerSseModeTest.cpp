// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/pipe/Source.h"
#include "tests/support/NetworkTest.h"
#include "utils/system/time.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Response.h"

#include <chrono>
#include <iostream>
#include <string>

namespace utils::system {
    time_t time(time_t* value) {
        if (value != nullptr) {
            *value = 0;
        }
        return 0;
    }
} // namespace utils::system

namespace {
    class EventSource final : public core::pipe::Source {
    public:
        ~EventSource() override;
        bool isOpen() override {
            return open;
        }
        void start() override {
            send("data: abc\n\n", 11);
            if (!hold) {
                eof();
            }
        }
        void suspend() override {
        }
        void resume() override {
        }
        void stop() override {
            open = false;
            ++stopped;
        }
        bool open = true;
        bool hold = false;
        int stopped = 0;
    };
    EventSource::~EventSource() = default;

    std::string request(bool close, bool acceptSse) {
        return std::string("POST / HTTP/1.1\r\nHost: localhost\r\nAccept: application/json") + (acceptSse ? ", text/event-stream" : "") +
               "\r\nContent-Type: application/json\r\nContent-Length: 2\r\nConnection: " + (close ? "close" : "keep-alive") + "\r\n\r\n{}";
    }
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    tests::support::Runtime runtime;
    tests::support::Peer peer;
    EventSource source;
    const std::string mode = argc > 1 ? argv[1] : "json-accept-sse";
    const bool json = mode == "json-accept-sse";
    const bool sequential = json || mode == "sse-keep-alive";
    source.hold = mode == "sse-without-accept";
    const std::string prefix = "HTTP/1.1 200 OK\r\nDate: Thu, 01 Jan 1970 00:00:00 GMT\r\nConnection: ";
    const std::string jsonHeaders = "Content-Length: 2\r\nContent-Type: application/json\r\nX-Powered-By: SNode.C\r\n\r\n{}";
    const std::string first = prefix + "keep-alive\r\n" +
                              (json ? jsonHeaders
                                    : "Content-Type: text/event-stream\r\nTransfer-Encoding: chunked\r\n"
                                      "X-Powered-By: SNode.C\r\n\r\nB\r\ndata: abc\n\n\r\n" +
                                          std::string(source.hold ? "" : "0\r\n\r\n"));
    const std::string expected = first + (sequential ? prefix + "close\r\n" + jsonHeaders : "");
    int requests = 0;
    bool accepted = false;
    bool closed = false;
    bool secondSent = false;
    bool firstSeen = false;
    bool heldOpen = false;
    double idle = 0;
    std::string wire;
    auto firstReceived = std::chrono::steady_clock::now();
    const auto started = firstReceived;
    double elapsed = 0;
    web::http::legacy::in::Server server("sse-mode", [&](const auto&, const auto& response) {
        ++requests;
        if (json || requests == 2) {
            response->type("application/json").send("{}");
        } else {
            response->type("text/event-stream");
            accepted = response->pipe(&source);
        }
    });
    server.getConfig()->Instance::forceUnrequired();
    server.getConfig()->setReadTimeout(utils::Timeval(0.1));
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& address, auto state) {
        tests::support::require(state == core::socket::State::OK, "listen");
        peer.connect(address.getPort());
        const std::string bytes = request(false, !source.hold);
        tests::support::require(peer.write(bytes) == static_cast<int>(bytes.size()), "write first JSON request");
    });
    result.expectTrue(tests::support::runUntil(
                          [&] {
                              return closed || heldOpen || elapsed >= 1;
                          },
                          [&] {
                              char bytes[4096];
                              const int count = peer.read(bytes, sizeof(bytes));
                              const auto now = std::chrono::steady_clock::now();
                              if (count > 0) {
                                  wire.append(bytes, static_cast<std::size_t>(count));
                              } else if (count == 0) {
                                  closed = true;
                              }
                              if (!firstSeen && wire == first) {
                                  firstReceived = now;
                                  firstSeen = true;
                              }
                              elapsed = std::chrono::duration<double>(now - started).count();
                              idle = std::chrono::duration<double>(now - firstReceived).count();
                              if (sequential && !secondSent && firstSeen) {
                                  const std::string next = request(true, true);
                                  tests::support::require(peer.write(next) == static_cast<int>(next.size()), "write second JSON request");
                                  secondSent = true;
                              }
                              heldOpen = source.hold && firstSeen && idle >= 0.35 && !closed && source.stopped == 0;
                          }),
                      "bounded connection observation completes");
    result.expectTrue(wire == expected, "status, headers, SSE framing, payload and termination match the exact wire");
    result.expectEqual(sequential ? 2 : 1, requests, "requests use one accepted connection");
    if (!json) {
        result.expectTrue(accepted, "response accepts the pipe Source");
    }
    if (mode == "sse-timeout") {
        result.expectTrue(firstSeen && closed && idle >= 0.05 && idle < 1, "Source EOF restores the configured 100 ms read timeout");
    } else if (source.hold) {
        result.expectTrue(heldOpen, "open SSE without event-stream Accept survives more than three configured read timeouts");
    } else {
        result.expectTrue(closed && secondSent, "second response completes before connection closure");
    }
    std::cout << "mode=" << mode << "; requests=" << requests << "; received=" << wire.size() << "; expected=" << expected.size()
              << "; exact=" << (wire == expected) << "; closed=" << closed << "; idle_ms=" << idle * 1000 << "; held_open=" << heldOpen
              << '\n';
    return result.processResult();
}
