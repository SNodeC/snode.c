#include "core/SNodeC.h"
#include "express/legacy/in6/WebApp.h"
#include "net/in6/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/legacy/in6/Client.h"

#include <cstdint>
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
    int handlerCount = 0;
};

std::string toString(const std::vector<char>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("Inet6ExpressTransportSmokeTest");
    } else {
        State state;

        core::SNodeC::init(argc, argv);

        const express::legacy::in6::WebApp app("ipv6-express-transport-smoke-server");
        web::http::legacy::in6::Client client(
            "ipv6-express-transport-smoke-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/express/transport-smoke";
                request->set("Connection", "close");
                request->end(
                    [&state](const auto&, const auto& response) {
                        ++state.clientResponseCount;
                        if (response->statusCode != "200" || response->get("X-Express-Transport") != "ipv6" ||
                            toString(response->body) != "express ipv6 transport ok") {
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

        app.get("/express/transport-smoke", [&state] APPLICATION(req, res) {
            ++state.handlerCount;
            res->status(200).set("X-Express-Transport", "ipv6").send("express ipv6 transport ok");
        });

        app.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();

        app.listen(net::in6::SocketAddress("::1", 0), [&client, &state](const net::in6::SocketAddress& socketAddress, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                const std::uint16_t effectivePort = socketAddress.getPort();
                if (effectivePort != 0) {
                    ++state.effectiveListenEndpointOkCount;
                    client.connect(net::in6::SocketAddress("::1", effectivePort), [&state](const net::in6::SocketAddress&, core::socket::State connectState) {
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

        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv6 legacy Express request");
        testResult.expectEqual(1, state.listenOkCount, "Express server listen callback reports OK exactly once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "Express server reports an effective IPv6 endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, "HTTP client connect callback reports OK exactly once");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP client reaches HTTP-connected callback exactly once");
        testResult.expectEqual(1, state.clientResponseCount, "HTTP client observes the Express response");
        testResult.expectEqual(0, state.parseErrorCount, "HTTP client reports no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, "IPv6 Express transport smoke reports no unexpected states");
        testResult.expectEqual(1, state.handlerCount, "Express route handler runs exactly once");

        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
