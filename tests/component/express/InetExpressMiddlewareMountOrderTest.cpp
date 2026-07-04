#include "core/SNodeC.h"
#include "express/legacy/in/WebApp.h"
#include "net/in/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/legacy/in/Client.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Case {
    std::string url;
    int status;
    std::string headerName;
    std::string headerValue;
    std::string body;
};

std::string toString(const std::vector<char>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

struct State { int listenOkCount=0; int effectiveListenEndpointOkCount=0; int clientConnectOkCount=0; int httpConnectedCount=0; int clientResponseCount=0; int parseErrorCount=0; int unexpectedStateCount=0; int appMiddlewareCount=0; int routerMiddlewareCount=0; int handlerCount=0; };

class CaseClientRunner {
public:
    CaseClientRunner(State& state, std::deque<Case> cases)
        : state(state)
        , cases(std::move(cases)) {
    }

    void run(std::uint16_t port) {
        this->port = port;
        dispatchNext();
    }

private:
    using Client = web::http::legacy::in::Client;
    using MasterRequest = Client::MasterRequest;

    void dispatchNext() {
        if (cases.empty()) {
            core::SNodeC::stop();
            return;
        }

        const std::size_t clientIndex = clients.size();
        const Case current = cases.front();
        cases.pop_front();

        clients.emplace_back(std::make_shared<Client>(
            "ipv4-express-continuation-client-" + std::to_string(clientIndex),
            [this, current](const std::shared_ptr<MasterRequest>& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = current.url;
                request->set("Connection", "close");
                request->end(
                    [this, current](const auto&, const auto& response) {
                        ++state.clientResponseCount;
                        if (response->statusCode != std::to_string(current.status)) {
                            ++state.unexpectedStateCount;
                        }
                        if (!current.headerName.empty() && response->get(current.headerName) != current.headerValue) {
                            ++state.unexpectedStateCount;
                        }
                        if (!current.body.empty() && toString(response->body) != current.body) {
                            ++state.unexpectedStateCount;
                        }
                        dispatchNext();
                    },
                    [this](const auto&, const std::string&) {
                        ++state.parseErrorCount;
                        dispatchNext();
                    });
            },
            [](const std::shared_ptr<MasterRequest>&) {
            }));

        const std::shared_ptr<Client>& client = clients.back();
        client->getConfig()->Instance::forceUnrequired();
        client->connect(net::in::SocketAddress("127.0.0.1", port), [this](const net::in::SocketAddress&, core::socket::State connectState) {
            if (connectState == core::socket::State::OK) {
                ++state.clientConnectOkCount;
            } else {
                ++state.unexpectedStateCount;
                core::SNodeC::stop();
            }
        });
    }

    State& state;
    std::uint16_t port = 0;
    std::deque<Case> cases;
    std::vector<std::shared_ptr<Client>> clients;
};

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetExpressMiddlewareMountOrderTest");
    } else {
        State state;

        core::SNodeC::init(argc, argv);

        const express::legacy::in::WebApp app("ipv4-expressmiddlewaremountorder-server");

        express::Router router;
        app.use([&state] MIDDLEWARE(req, res, next) {
            ++state.appMiddlewareCount;
            req->setAttribute<std::string>("app-before", "trace");
            next();
        });
        router.use([&state] MIDDLEWARE(req, res, next) {
            ++state.routerMiddlewareCount;
            std::string trace;
            req->getAttribute<std::string>([&trace](const std::string& value) { trace = value; }, "trace");
            req->setAttribute<std::string>(trace + ",router-before", "trace", true);
            next();
        });
        router.get("/status", [&state] APPLICATION(req, res) {
            ++state.handlerCount;
            std::string trace;
            req->getAttribute<std::string>([&trace](const std::string& value) { trace = value; }, "trace");
            trace += ",handler";
            res->status(200).set("X-Express-Mount-Order", trace).send(trace);
        });
        app.use("/api", router);

        app.getConfig()->Instance::forceUnrequired();

        CaseClientRunner caseClientRunner(state, {{{"/api/status", 200, "X-Express-Mount-Order", "app-before,router-before,handler", "app-before,router-before,handler"}, {"/outside", 404, "", "", ""}}});
        app.listen(net::in::SocketAddress("127.0.0.1", 0), [&caseClientRunner, &state](const net::in::SocketAddress& socketAddress, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                const std::uint16_t effectivePort = socketAddress.getPort();
                if (effectivePort != 0) {
                    ++state.effectiveListenEndpointOkCount;
                    caseClientRunner.run(effectivePort);
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

        testResult.expectEqual(0, startResult, "event loop stops successfully");
        testResult.expectEqual(1, state.listenOkCount, "Express server listen callback reports OK exactly once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "Express server reports an effective IPv4 endpoint");
        testResult.expectEqual(2, state.clientConnectOkCount, "HTTP client connect callbacks report OK once per request");
        testResult.expectEqual(2, state.httpConnectedCount, "HTTP client reaches HTTP-connected callback once per request");
        testResult.expectEqual(2, state.clientResponseCount, "HTTP client observes all Express responses");
        testResult.expectEqual(0, state.parseErrorCount, "HTTP client reports no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, "test cases report no unexpected states");
        testResult.expectEqual(2, state.appMiddlewareCount, "application middleware runs for both requests");
        testResult.expectEqual(1, state.routerMiddlewareCount, "mounted router middleware runs only below mount path");
        testResult.expectEqual(1, state.handlerCount, "mounted route handler runs exactly once");

        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
