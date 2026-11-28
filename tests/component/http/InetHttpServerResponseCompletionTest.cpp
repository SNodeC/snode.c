#include "core/pipe/Source.h"
#include "support/TestResult.h"
#include "tests/support/NetworkTest.h"
#include "utils/system/time.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Response.h"

#include <iostream>

namespace utils::system {
    time_t time(time_t* value) {
        if (value != nullptr) {
            *value = 0;
        }
        return 0;
    }
} // namespace utils::system

namespace {
    class FragmentSource : public core::pipe::Source {
    public:
        bool isOpen() override {
            return open;
        }

        void start() override {
            send("abc", 3);
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
        int stopped = 0;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    tests::support::Runtime runtime;
    tests::support::Peer peer;
    FragmentSource source;
    const std::string mode = argc > 1 ? argv[1] : "keep-alive";
    const std::string prefix =
        std::string(mode == "http10" ? "HTTP/1.0" : "HTTP/1.1") + " 200 OK\r\nDate: Thu, 01 Jan 1970 00:00:00 GMT\r\nConnection: ";
    std::string expected = prefix + (mode == "keep-alive" ? "keep-alive" : "close") + "\r\n";
    if (mode == "fixed" || mode == "http10") {
        expected += mode == "http10" ? "Content-Length: 3\r\nContent-Type: text/plain\r\nX-Powered-By: SNode.C\r\n\r\nabc"
                                     : "Content-Length: 7\r\nContent-Type: text/plain\r\nX-Powered-By: SNode.C\r\n\r\nabctail";
    } else {
        expected += "Content-Type: text/plain\r\n";
        if (mode == "trailers") {
            expected += "Trailer: Digest\r\n";
        }
        expected += "Transfer-Encoding: chunked\r\nX-Powered-By: SNode.C\r\n\r\n";
        if (mode == "manual") {
            expected += "6\r\nhello \r\n8\r\nchunked \r\n8\r\nresponse\r\n0\r\n\r\n";
        } else if (mode != "head") {
            if (mode != "empty") {
                expected += "3\r\nabc\r\n";
            }
            if (mode == "pipe" || mode == "trailers") {
                expected += "4\r\ntail\r\n";
            }
            expected += mode == "trailers" ? "0\r\nDigest: done\r\n\r\n" : "0\r\n\r\n";
        }
    }
    const std::string first = expected;
    if (mode == "keep-alive") {
        expected += prefix + "close\r\nContent-Length: 6\r\nContent-Type: text/plain\r\nX-Powered-By: SNode.C\r\n\r\nsecond";
    }
    web::http::server::Response disconnected(nullptr);
    disconnected.sendHeader().sendFragment("ignored");
    disconnected.end();
    testResult.expectTrue(!disconnected.pipe(&source), "disconnected operations do not attach a source");
    int requests = 0;
    bool accepted = false;
    bool closed = false;
    bool secondSent = false;
    std::string wire;
    web::http::legacy::in::Server server("response-completion", [&](const auto&, const auto& response) {
        ++requests;
        response->type("text/plain");
        if (requests == 2) {
            response->send("second");
        } else if (mode == "manual") {
            response->set("Transfer-Encoding", "chunked")
                .sendHeader()
                .sendFragment("hello ")
                .sendFragment("chunked ")
                .sendFragment("response")
                .sendFragment("");
        } else if (mode == "keep-alive" || mode == "empty" || mode == "head" || mode == "http10") {
            if (mode == "http10") {
                response->set("Content-Length", "3");
            } else {
                response->set("Transfer-Encoding", "chunked");
            }
            response->sendHeader();
            if (mode != "empty") {
                response->sendFragment("abc");
            }
            response->end();
        } else {
            if (mode == "fixed") {
                response->set("Content-Length", "7");
            } else if (mode == "trailers") {
                response->setTrailer("Digest", "done");
            }
            accepted = response->pipe(&source);
            response->send("tail");
        }
    });
    server.getConfig()->Instance::forceUnrequired();
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& address, auto state) {
        tests::support::require(state == core::socket::State::OK, "listen");
        peer.connect(address.getPort());
        const std::string request = std::string(mode == "head" ? "HEAD" : "GET") + " / " + (mode == "http10" ? "HTTP/1.0" : "HTTP/1.1") +
                                    "\r\nHost: localhost\r\nConnection: " + std::string(mode == "keep-alive" ? "keep-alive" : "close") +
                                    "\r\n\r\n";
        tests::support::require(peer.write(request) == static_cast<int>(request.size()), "write request");
    });
    testResult.expectTrue(tests::support::runUntil(
                              [&] {
                                  return closed;
                              },
                              [&] {
                                  char bytes[4096];
                                  const int count = peer.read(bytes, sizeof(bytes));
                                  if (count > 0) {
                                      wire.append(bytes, static_cast<std::size_t>(count));
                                  } else if (count == 0) {
                                      closed = true;
                                  }
                                  if (mode == "keep-alive" && !secondSent && wire == first) {
                                      const std::string request = "GET /second HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
                                      tests::support::require(peer.write(request) == static_cast<int>(request.size()),
                                                              "write second request");
                                      secondSent = true;
                                  } else if (mode == "manual" && wire == expected) {
                                      peer.close();
                                      closed = true;
                                  }
                              }),
                          "the exact response sequence reaches one raw connection");
    testResult.expectTrue(wire == expected, "headers, framing, payload and completion bytes are exact");
    testResult.expectEqual(mode == "keep-alive" ? 2 : 1, requests, "both sequential requests use the same accepted connection");
    if (mode == "pipe" || mode == "trailers" || mode == "fixed") {
        testResult.expectTrue(accepted, "the source is accepted");
        testResult.expectEqual(1, source.stopped, "completion stops the attached source once");
    }
    std::cout << "mode=" << mode << "; received=" << wire.size() << "; expected=" << expected.size() << "; exact=" << (wire == expected)
              << '\n';
    return testResult.processResult();
}
