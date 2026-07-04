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
#include <vector>

namespace {

struct Case {
    std::string name;
    std::string method;
    std::string url;
    int status;
    std::string headerName;
    std::string headerValue;
    std::string body;
};

struct State {
    int listenOkCount = 0;
    int effectiveListenEndpointOkCount = 0;
    int clientConnectOkCount = 0;
    int httpConnectedCount = 0;
    int clientResponseCount = 0;
    int parseErrorCount = 0;
    int unexpectedStateCount = 0;

    int exactRouteCount = 0;
    int parameterRouteCount = 0;
    int queryRouteCount = 0;
    int middlewareFirstCount = 0;
    int middlewareSecondCount = 0;
    int middlewareRouteCount = 0;

    bool exactRouteObservedRequest = false;
    bool parameterRouteObservedId = false;
    bool queryRouteObservedMode = false;
    bool middlewareObservedOrder = false;
};

std::string toString(const std::vector<char>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

class CaseClientRunner {
public:
    explicit CaseClientRunner(State& state)
        : state(state)
        , cases({{"exact route", "GET", "/express/basic", 200, "X-Express-Test", "exact", "express basic ok"},
                 {"path parameter", "GET", "/express/items/42", 200, "X-Express-Param", "42", "item id=42"},
                 {"query visibility", "GET", "/express/query?mode=test", 200, "X-Express-Query", "test", "query mode=test"},
                 {"method mismatch fallback", "POST", "/express/get-only", 404, "", "", ""},
                 {"404 fallback", "GET", "/express/missing", 404, "", "", ""},
                 {"middleware order and header", "GET", "/express/middleware", 200, "X-Express-Middleware", "first,second", "middleware order=first,second"}}) {
    }

    void run(const std::uint16_t port) {
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
            "ipv4-express-routing-middleware-client-" + std::to_string(clientIndex),
            [this, current](const std::shared_ptr<MasterRequest>& request) {
                ++state.httpConnectedCount;
                request->method = current.method;
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
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetExpressRoutingMiddlewareTest");
    } else {
        State state;

        core::SNodeC::init(argc, argv);

        const express::legacy::in::WebApp app("ipv4-express-routing-middleware-server");

        app.get("/express/basic", [&state] APPLICATION(req, res) {
            ++state.exactRouteCount;
            state.exactRouteObservedRequest = req->method == "GET" && req->path == "/express/basic";
            res->status(200).set("X-Express-Test", "exact").send("express basic ok");
        });

        app.get("/express/items/:id", [&state] APPLICATION(req, res) {
            ++state.parameterRouteCount;
            state.parameterRouteObservedId = req->param("id") == "42";
            res->status(200).set("X-Express-Param", req->param("id")).send("item id=" + req->param("id"));
        });

        app.get("/express/query", [&state] APPLICATION(req, res) {
            ++state.queryRouteCount;
            state.queryRouteObservedMode = req->query("mode") == "test";
            res->status(200).set("X-Express-Query", req->query("mode")).send("query mode=" + req->query("mode"));
        });

        app.get("/express/get-only", [] APPLICATION(req, res) {
            res->status(200).send("get-only route should not handle POST");
        });

        app.get(
            "/express/middleware",
            [&state] MIDDLEWARE(req, res, next) {
                ++state.middlewareFirstCount;
                req->setAttribute<std::string>("first", "express-middleware-order");
                next();
            },
            [&state] MIDDLEWARE(req, res, next) {
                ++state.middlewareSecondCount;
                std::string order;
                req->getAttribute<std::string>(
                    [&order](const std::string& value) {
                        order = value;
                    },
                    "express-middleware-order");
                req->setAttribute<std::string>(order + ",second", "express-middleware-order", true);
                res->set("X-Express-Middleware", order + ",second");
                next();
            },
            [&state] APPLICATION(req, res) {
                ++state.middlewareRouteCount;
                std::string order;
                req->getAttribute<std::string>(
                    [&order](const std::string& value) {
                        order = value;
                    },
                    "express-middleware-order");
                state.middlewareObservedOrder = order == "first,second";
                res->status(200).send("middleware order=" + order);
            });

        app.getConfig()->Instance::forceUnrequired();

        CaseClientRunner caseClientRunner(state);
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

        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv4 legacy Express requests");
        testResult.expectEqual(1, state.listenOkCount, "Express server listen callback reports OK exactly once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "Express server reports an effective IPv4 endpoint");
        testResult.expectEqual(6, state.clientConnectOkCount, "HTTP client connect callbacks report OK once per Express request");
        testResult.expectEqual(6, state.httpConnectedCount, "HTTP client reaches HTTP-connected callback once per Express request");
        testResult.expectEqual(6, state.clientResponseCount, "HTTP client observes all Express responses");
        testResult.expectEqual(0, state.parseErrorCount, "HTTP client reports no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, "Express routing and middleware cases report no unexpected states");
        testResult.expectEqual(1, state.exactRouteCount, "exact route handler runs exactly once");
        testResult.expectTrue(state.exactRouteObservedRequest, "exact route handler observes deterministic GET request path");
        testResult.expectEqual(1, state.parameterRouteCount, "parameter route handler runs exactly once");
        testResult.expectTrue(state.parameterRouteObservedId, "parameter route handler observes id parameter");
        testResult.expectEqual(1, state.queryRouteCount, "query route handler runs exactly once");
        testResult.expectTrue(state.queryRouteObservedMode, "query route handler observes deterministic query value");
        testResult.expectEqual(1, state.middlewareFirstCount, "first middleware runs exactly once");
        testResult.expectEqual(1, state.middlewareSecondCount, "second middleware runs exactly once");
        testResult.expectEqual(1, state.middlewareRouteCount, "middleware route handler runs exactly once");
        testResult.expectTrue(state.middlewareObservedOrder, "route observes middleware execution order");

        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
