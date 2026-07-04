#include "core/SNodeC.h"
#include "express/legacy/in/WebApp.h"
#include "net/in/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/legacy/in/Client.h"

#include <string>
#include <vector>

namespace {

struct State {
    int listenOkCount = 0;
    int effectiveListenEndpointOkCount = 0;
    int clientConnectOkCount = 0;
    int httpConnectedCount = 0;
    int clientResponseCount = 0;
    int parseErrorCount = 0;
    int unexpectedStateCount = 0;
    int middlewareCount = 0;
    int downstreamHandlerCount = 0;
};

std::string toString(const std::vector<char>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetExpressMiddlewareShortCircuitTest");
    } else {
        State state;

        core::SNodeC::init(argc, argv);

        const express::legacy::in::WebApp app("ipv4-express-middleware-short-circuit-server");
        web::http::legacy::in::Client client(
            "ipv4-express-middleware-short-circuit-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/express/short-circuit";
                request->set("Connection", "close");
                request->end(
                    [&state](const auto&, const auto& response) {
                        ++state.clientResponseCount;
                        if (response->statusCode != "403" || response->get("X-Express-Short-Circuit") != "middleware" ||
                            toString(response->body) != "middleware stopped request") {
                            ++state.unexpectedStateCount;
                        }
                        core::SNodeC::stop();
                    },
                    [&state](const auto&, const std::string&) {
                        ++state.parseErrorCount;
                        core::SNodeC::stop();
                    });
            },
            [](const auto&) {
            });

        app.get(
            "/express/short-circuit",
            [&state] MIDDLEWARE(req, res, next) {
                ++state.middlewareCount;
                res->status(403).set("X-Express-Short-Circuit", "middleware").send("middleware stopped request");
            },
            [&state] APPLICATION(req, res) {
                ++state.downstreamHandlerCount;
                res->status(200).send("downstream handler should not run");
            });

        app.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();

        app.listen(net::in::SocketAddress("127.0.0.1", 0), [&client, &state](const net::in::SocketAddress& socketAddress, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                const std::uint16_t effectivePort = socketAddress.getPort();
                if (effectivePort != 0) {
                    ++state.effectiveListenEndpointOkCount;
                    client.connect(net::in::SocketAddress("127.0.0.1", effectivePort), [&state](const net::in::SocketAddress&, core::socket::State connectState) {
                        if (connectState == core::socket::State::OK) {
                            ++state.clientConnectOkCount;
                        } else {
                            ++state.unexpectedStateCount;
                            core::SNodeC::stop();
                        }
                    });
                } else {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            } else {
                ++state.unexpectedStateCount;
                core::SNodeC::stop();
            }
        });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv4 legacy Express middleware response");
        testResult.expectEqual(1, state.listenOkCount, "Express server listen callback reports OK exactly once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "Express server reports an effective IPv4 endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, "HTTP client connect callback reports OK exactly once");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP client reaches HTTP-connected callback exactly once");
        testResult.expectEqual(1, state.clientResponseCount, "HTTP client observes the middleware response");
        testResult.expectEqual(0, state.parseErrorCount, "HTTP client reports no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, "middleware short-circuit case reports no unexpected states");
        testResult.expectEqual(1, state.middlewareCount, "short-circuit middleware runs exactly once");
        testResult.expectEqual(0, state.downstreamHandlerCount, "downstream Express handler does not run after middleware response");

        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
