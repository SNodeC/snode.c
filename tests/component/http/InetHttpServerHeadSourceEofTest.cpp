#include "core/pipe/Source.h"
#include "tests/support/NetworkTest.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Response.h"

#include <iomanip>
#include <iostream>
#include <string>

namespace {
    class EofSource : public core::pipe::Source {
    public:
        bool isOpen() override {
            return open;
        }

        void start() override {
            eof();
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

int main() {
    tests::support::TestResult testResult;
    tests::support::Runtime runtime;
    tests::support::Peer peer;
    EofSource source;
    bool accepted = false;
    bool closed = false;
    std::string wire;
    web::http::legacy::in::Server server("head-source-eof", [&](const auto&, const auto& response) {
        response->set("Transfer-Encoding", "chunked");
        accepted = response->pipe(&source);
    });
    server.getConfig()->Instance::forceUnrequired();
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& address, auto state) {
        tests::support::require(state == core::socket::State::OK, "listen");
        peer.connect(address.getPort());
        const std::string request = "HEAD / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
        tests::support::require(peer.write(request) == static_cast<int>(request.size()), "write HEAD");
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
                              }),
                          "HEAD source EOF drains and closes the response");
    const std::size_t separator = wire.find("\r\n\r\n");
    testResult.expectTrue(separator != std::string::npos, "HEAD response headers reach the peer");
    const std::string body = separator == std::string::npos ? wire : wire.substr(separator + 4);
    std::cout << "HEAD body bytes=" << body.size() << "; hex=";
    for (const unsigned char byte : body) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
    }
    std::cout << std::dec << '\n';
    testResult.expectTrue(body.empty(), "HEAD source EOF emits no chunk terminator or body bytes");
    testResult.expectTrue(accepted, "HEAD source is accepted");
    testResult.expectEqual(1, source.stopped, "HEAD source is stopped once");
    return testResult.processResult();
}
