#include "core/SNodeC.h"
#include "express/legacy/un/WebApp.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/legacy/un/Client.h"

#include <filesystem>
#include <string>
#include <unistd.h>
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

std::string makeSocketPath() {
    return "/tmp/snodec-express-component-" + std::to_string(::getpid()) + ".sock";
}

std::string toString(const std::vector<char>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("UnixExpressTransportSmokeTest");
    } else {
        State state;
        const std::string socketPath = makeSocketPath();
        std::filesystem::remove(socketPath);

        core::SNodeC::init(argc, argv);

        const express::legacy::un::WebApp app("unix-express-transport-smoke-server");
        web::http::legacy::un::Client client(
            "unix-express-transport-smoke-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/express/transport-smoke";
                request->set("Connection", "close");
                request->end(
                    [&state](const auto&, const auto& response) {
                        ++state.clientResponseCount;
                        if (response->statusCode != "200" || response->get("X-Express-Transport") != "unix" ||
                            toString(response->body) != "express unix transport ok") {
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
            res->status(200).set("X-Express-Transport", "unix").send("express unix transport ok");
        });

        app.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();

        app.listen(socketPath, [&client, &socketPath, &state](const auto&, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                ++state.effectiveListenEndpointOkCount;
                client.connect(socketPath, [&state](const auto&, core::socket::State connectState) {
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
        });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        testResult.expectEqual(0, startResult, "event loop stops successfully after Unix-domain legacy Express request");
        testResult.expectEqual(1, state.listenOkCount, "Express server listen callback reports OK exactly once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "Express server reports an effective Unix-domain endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, "HTTP client connect callback reports OK exactly once");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP client reaches HTTP-connected callback exactly once");
        testResult.expectEqual(1, state.clientResponseCount, "HTTP client observes the Express response");
        testResult.expectEqual(0, state.parseErrorCount, "HTTP client reports no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, "Unix-domain Express transport smoke reports no unexpected states");
        testResult.expectEqual(1, state.handlerCount, "Express route handler runs exactly once");

        result = testResult.processResult();
        core::SNodeC::free();
        std::filesystem::remove(socketPath);
    }

    return result;
}
