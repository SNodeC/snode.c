/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/SNodeC.h"
#include "core/EventReceiver.h"
#include "core/timer/Timer.h"
#include "express/legacy/in/WebApp.h"
#include "log/Logger.h"
#include "web/http/legacy/in/Client.h"

#include <cstddef>
#include <deque>

using express::Router;

struct TestCase {
    std::string name;
    std::string url;
    int expectedStatus;
    std::string expectedBody;
    std::string connection;
};

class NextTester {
public:
    NextTester()
        : testCases({{"next() middleware chain", "/next/basic", 200, "next() reached final handler", "keep-alive"},
                     {"next() asynchronous middleware chain", "/next/async/basic", 200, "next(async) reached final handler", "keep-alive"},
                     {"next('route') fallback", "/next/route/0", 200, "next(route) fallback for id=0", "keep-alive"},
                     {"next('route') primary", "/next/route/7", 200, "next(route) primary for id=7", "keep-alive"},
                     {"next('route') asynchronous fallback", "/next/async/route/0", 200, "next(async route) fallback for id=0", "keep-alive"},
                     {"next('route') asynchronous primary", "/next/async/route/7", 200, "next(async route) primary for id=7", "keep-alive"},
                     {"next('router') fallback", "/next/router/resource?allow=false", 403, "next(router) denied by fallback", "keep-alive"},
                     {"next('router') inside router", "/next/router/resource?allow=true", 200, "next(router) router handler", "keep-alive"},
                     {"next('router') asynchronous fallback", "/next/async/router/resource?allow=false", 403,
                      "next(async router) denied by fallback", "keep-alive"},
                     {"next('router') asynchronous inside router", "/next/async/router/resource?allow=true", 200,
                      "next(async router) router handler", "close"}}) {
    }

    void run() {
        client = std::make_shared<Client>( //
            "testexpressnext-client",
            [this](const std::shared_ptr<MasterRequest>& connectedRequest) {
                masterRequest = connectedRequest;
                dispatchNextRequest();
            },
            [](const std::shared_ptr<MasterRequest>&) {
            });

        client->connect( //
            "localhost",
            18080,
            [this](const Client::SocketAddress&, const core::socket::State& state) {
                if (state != core::socket::State::OK) {
                    ++failures;
                    LOG(ERROR) << "FAIL: connect failed: " << state.what();
                    core::timer::Timer::singleshotTimer(
                        [] {
                            core::SNodeC::stop();
                        },
                        1);
                }
            });
    }

    std::size_t getFailures() const {
        return failures;
    }

private:
    using Client = web::http::legacy::in::Client;
    using MasterRequest = Client::MasterRequest;
    using Request = Client::Request;
    using Response = Client::Response;

    void dispatchNextRequest() {
        if (testCases.empty()) {
            LOG(INFO) << "All express next() tests executed. failures=" << failures;
            if (masterRequest && masterRequest->isConnected()) {
                masterRequest->disconnect();
            }
            core::timer::Timer::singleshotTimer(
                [] {
                    core::SNodeC::stop();
                },
                1);
            return;
        }

        if (!masterRequest || !masterRequest->isConnected()) {
            ++failures;
            LOG(ERROR) << "FAIL: master request not connected";
            core::timer::Timer::singleshotTimer(
                [] {
                    core::SNodeC::stop();
                },
                1);
            return;
        }

        const TestCase current = testCases.front();
        testCases.pop_front();

        masterRequest->init();
        masterRequest->method = "GET";
        masterRequest->url = current.url;
        masterRequest->set("Connection", current.connection);
        masterRequest->end(
            [this, current]([[maybe_unused]] const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                const std::string body(res->body.begin(), res->body.end());
                const bool statusOk = std::stoi(res->statusCode) == current.expectedStatus;
                const bool bodyOk = body.find(current.expectedBody) != std::string::npos;

                if (statusOk && bodyOk) {
                    LOG(INFO) << "PASS: " << current.name << " status=" << res->statusCode << " body='" << body << "'";
                } else {
                    ++failures;
                    LOG(ERROR) << "FAIL: " << current.name << " expected status=" << current.expectedStatus << " expected body fragment='"
                               << current.expectedBody << "'"
                               << " got status=" << res->statusCode << " body='" << body << "'";
                }

                dispatchNextRequest();
            },
            [this, current]([[maybe_unused]] const std::shared_ptr<Request>& req, const std::string& reason) {
                ++failures;
                LOG(ERROR) << "FAIL: " << current.name << " parse-error: " << reason;
                dispatchNextRequest();
            });
    }

    std::deque<TestCase> testCases;
    std::shared_ptr<Client> client;
    std::shared_ptr<MasterRequest> masterRequest;
    std::size_t failures = 0;
};

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const express::legacy::in::WebApp app("testexpressnext-server");

    app.get(
        "/next/basic",
        [] MIDDLEWARE(req, res, next) {
            req->setAttribute<std::string>("middleware-seen", "yes");
            next();
        },
        [] APPLICATION(req, res) {
            std::string marker = "no";
            req->getAttribute<std::string>(
                [&](const std::string& value) {
                    marker = value;
                },
                "middleware-seen");
            res->status(200).send("next() reached final handler (middleware=" + marker + ")");
        });

    app.get(
        "/next/async/basic",
        [] MIDDLEWARE(req, res, next) {
            req->setAttribute<std::string>("middleware-seen-async", "yes");
            core::EventReceiver::atNextTick([next] {
                next();
            });
        },
        [] APPLICATION(req, res) {
            std::string marker = "no";
            req->getAttribute<std::string>(
                [&](const std::string& value) {
                    marker = value;
                },
                "middleware-seen-async");
            res->status(200).send("next(async) reached final handler (middleware=" + marker + ")");
        });

    app.get(
        "/next/route/:id(\\d+)",
        [] MIDDLEWARE(req, res, next) {
            if (req->params["id"] == "0") {
                next("route");
                return;
            }
            next();
        },
        [] APPLICATION(req, res) {
            res->status(200).send("next(route) primary for id=" + req->params["id"]);
        });

    app.get("/next/route/:id(\\d+)", [] APPLICATION(req, res) {
        res->status(200).send("next(route) fallback for id=" + req->params["id"]);
    });

    app.get(
        "/next/async/route/:id(\\d+)",
        [] MIDDLEWARE(req, res, next) {
            if (req->params["id"] == "0") {
                core::EventReceiver::atNextTick([next] {
                    next("route");
                });
                return;
            }
            core::EventReceiver::atNextTick([next] {
                next();
            });
        },
        [] APPLICATION(req, res) {
            res->status(200).send("next(async route) primary for id=" + req->params["id"]);
        });

    app.get("/next/async/route/:id(\\d+)", [] APPLICATION(req, res) {
        res->status(200).send("next(async route) fallback for id=" + req->params["id"]);
    });

    const Router guarded;
    guarded.use([] MIDDLEWARE(req, res, next) {
        if (req->queries["allow"] != "true") {
            next("router");
            return;
        }
        next();
    });
    guarded.get("/resource", [] APPLICATION(req, res) {
        res->status(200).send("next(router) router handler");
    });

    app.use("/next/router", guarded);
    app.get("/next/router/:rest(.*)", [] APPLICATION(req, res) {
        res->status(403).send("next(router) denied by fallback");
    });

    const Router guardedAsync;
    guardedAsync.use([] MIDDLEWARE(req, res, next) {
        if (req->queries["allow"] != "true") {
            core::EventReceiver::atNextTick([next] {
                next("router");
            });
            return;
        }
        core::EventReceiver::atNextTick([next] {
            next();
        });
    });
    guardedAsync.get("/resource", [] APPLICATION(req, res) {
        res->status(200).send("next(async router) router handler");
    });

    app.use("/next/async/router", guardedAsync);
    app.get("/next/async/router/:rest(.*)", [] APPLICATION(req, res) {
        res->status(403).send("next(async router) denied by fallback");
    });

    app.getConfig()->setReuseAddress();

    app.listen(18080, [](const express::legacy::in::WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
        switch (state) {
            case core::socket::State::OK:
                VLOG(1) << "testexpressnext listening on '" << socketAddress.toString() << "'";
                break;
            case core::socket::State::DISABLED:
                VLOG(1) << "testexpressnext disabled";
                break;
            case core::socket::State::ERROR:
                LOG(ERROR) << "testexpressnext " << socketAddress.toString() << ": " << state.what();
                break;
            case core::socket::State::FATAL:
                LOG(FATAL) << "testexpressnext " << socketAddress.toString() << ": " << state.what();
                break;
        }
    });

    NextTester nextTester;
    core::timer::Timer::singleshotTimer(
        [&nextTester] {
            nextTester.run();
        },
        1);

    const int rc = core::SNodeC::start();
    if (nextTester.getFailures() > 0) {
        LOG(ERROR) << "testexpressnext finished with failures=" << nextTester.getFailures();
        return 1;
    }

    LOG(INFO) << "testexpressnext finished successfully";
    return rc;
}
