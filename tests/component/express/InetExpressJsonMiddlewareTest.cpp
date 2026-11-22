#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "tests/support/NetworkTest.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    tests::support::Runtime runtime;
    tests::support::Peer peer;

    struct Case {
        std::string body;
        std::string expected;
        std::string contentType;
    };

    std::vector<Case> cases{{"{\"value\":42}", "{\"value\":42}", "text/plain"},
                            {"{bad", "absent", "application/json"},
                            {"", "absent", "application/json"},
                            {"[1,2]", "[1,2]", "application/json"},
                            {"true", "true", ""},
                            {"{\"replacement\":true}", "42", "application/problem+json"}};
    if (argc > 1 && std::string(argv[1]) == "--probe") {
        cases.resize(1);
    }
    int downstream = 0;
    bool eof = false;
    std::string wire;
    const express::legacy::in::WebApp app("json-middleware-test");
    app.use([] MIDDLEWARE(req, res, next) {
        if (req->url == "/5") {
            req->setAttribute<nlohmann::json>(nlohmann::json(42));
        }
        next();
    });
    app.use(express::middleware::JsonMiddleware());
    app.post("/:index", [&] APPLICATION(req, res) {
        const std::size_t index = std::stoul(req->url.substr(1));
        std::string attribute = "absent";
        req->getAttribute<nlohmann::json>([&](const nlohmann::json& json) {
            attribute = json.dump();
        });
        testResult.expectTrue(index < cases.size() && attribute == cases[index].expected,
                              "JSON attribute preserves the existing value/absence");
        ++downstream;
        res->type("text/plain").send("case=" + std::to_string(index) + ";attribute=" + attribute);
    });
    app.getConfig()->Instance::forceUnrequired();
    app.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& address, auto state) {
        tests::support::require(state == core::socket::State::OK, "listen");
        peer.connect(address.getPort());
        std::string requests;
        for (std::size_t index = 0; index < cases.size(); ++index) {
            requests += "POST /" + std::to_string(index) +
                        " HTTP/1.1\r\nHost: localhost\r\nConnection: " + (index + 1 == cases.size() ? "close" : "keep-alive") +
                        "\r\nContent-Length: " + std::to_string(cases[index].body.size()) + "\r\n";
            if (!cases[index].contentType.empty()) {
                requests += "Content-Type: " + cases[index].contentType + "\r\n";
            }
            requests += "\r\n" + cases[index].body;
        }
        tests::support::require(peer.write(requests) == static_cast<int>(requests.size()), "write requests");
    });
    testResult.expectTrue(tests::support::runUntil(
                              [&] {
                                  return eof;
                              },
                              [&] {
                                  char bytes[4096];
                                  const int count = peer.read(bytes, sizeof(bytes));
                                  if (count > 0) {
                                      wire.append(bytes, static_cast<std::size_t>(count));
                                  } else if (count == 0) {
                                      eof = true;
                                  }
                              }),
                          "JSON middleware responses reach the raw peer");
    testResult.expectEqual(static_cast<int>(cases.size()), downstream, "next reaches the handler exactly once per valid or invalid body");
    for (std::size_t index = 0; index < cases.size(); ++index) {
        testResult.expectTrue(wire.find("case=" + std::to_string(index) + ";attribute=" + cases[index].expected) != std::string::npos,
                              "the peer receives the unchanged JSON attribute result");
    }
    return testResult.processResult();
}
