#include "core/pipe/Source.h"
#include "tests/support/NetworkTest.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Response.h"

#include <cerrno>
class FailingSource : public core::pipe::Source {
public:
    bool isOpen() override {
        return open;
    }
    void start() override {
        ++started;
        send("partial", 7);
    }
    void fail() {
        error(EIO);
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
    int started = 0, stopped = 0;
};
int main() {
    using namespace tests::support;
    TestResult result;
    Runtime runtime;
    Peer peer;
    FailingSource source;
    int requests = 0;
    bool accepted = false, eof = false;
    web::http::legacy::in::Server server("source-failure", [&](const auto&, const auto& response) {
        ++requests;
        response->status(200).type("text/plain").set("Content-Length", "100");
        accepted = response->pipe(&source);
    });
    server.getConfig()->Instance::forceUnrequired();
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& addr, auto status) {
        require(status == core::socket::State::OK, "listen");
        peer.connect(addr.getPort());
        const std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        require(peer.write(request) == static_cast<int>(request.size()), "request");
    });
    std::string wire;
    result.expectTrue(runUntil(
                          [&] {
                              return eof;
                          },
                          [&] {
                              if (peer.fd < 0)
                                  return;
                              char data[4096];
                              int n = peer.read(data, sizeof(data));
                              if (n > 0)
                                  wire.append(data, static_cast<std::size_t>(n));
                              if (n == 0)
                                  eof = true;
                              // Fail only after the peer has received the initial body fragment.
                              if (source.open && wire.find("\r\n\r\npartial") != std::string::npos)
                                  source.fail();
                          }),
                      "source failure closes the real HTTP connection");
    result.expectEqual(1, requests, "one request reaches application");
    result.expectTrue(accepted, "source accepted through Response::pipe");
    result.expectEqual(1, source.started, "source started once");
    result.expectEqual(1, source.stopped, "failed source stopped once");
    result.expectTrue(wire.find("HTTP/1.1 200") != std::string::npos && wire.find("Content-Length: 100") != std::string::npos,
                      "response headers reached peer");
    auto body = wire.find("\r\n\r\n");
    result.expectTrue(body != std::string::npos && wire.substr(body + 4) == "partial", "failed body is not reported as complete");
    return result.processResult();
}
