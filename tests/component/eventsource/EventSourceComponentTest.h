/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#ifndef TESTS_COMPONENT_EVENTSOURCE_EVENTSOURCECOMPONENTTEST_H
#define TESTS_COMPONENT_EVENTSOURCE_EVENTSOURCECOMPONENTTEST_H

#include "core/SNodeC.h"
#include "net/in/SocketAddress.h"
#include "support/Phase3SemanticLogCapture.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/CiStringMap.h"
#include "web/http/client/tools/EventSource.h"
#include "web/http/legacy/in/EventSource.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Request.h"
#include "web/http/server/Response.h"
#include "web/http/server/SocketContext.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace tests::component::eventsource {

    struct ExpectedEvent {
        std::string type;
        std::string id;
        std::string data;
    };

    struct State {
        int listenOkCount = 0;
        int effectiveListenEndpointOkCount = 0;
        int eventSourceOpenCount = 0;
        int serverRequestCount = 0;
        int clientEventCount = 0;
        int parseErrorCount = 0;
        int unexpectedDisconnectCount = 0;
        int unexpectedStateCount = 0;

        bool serverSawPath = false;
        bool serverSawAccept = false;
        bool stoppedAfterExpectedEvents = false;
        bool closeInvoked = false;
        bool allowReconnect = false;

        std::vector<ExpectedEvent> receivedEvents;
        std::shared_ptr<web::http::client::tools::EventSource> eventSource;
    };

    template <typename Response>
    void sendSseEvent(const std::shared_ptr<Response>& response, const ExpectedEvent& event) {
        response->sendFragment("event: " + event.type);
        response->sendFragment("id: " + event.id);
        response->sendFragment("data: " + event.data);
        response->sendFragment("");
    }

    template <typename Response>
    void sendSseMultiLineDataEvent(const std::shared_ptr<Response>& response, const std::string& type, const std::string& id) {
        response->sendFragment("event: " + type);
        response->sendFragment("id: " + id);
        response->sendFragment("data: line-1");
        response->sendFragment("data: line-2");
        response->sendFragment("data: line-3");
        response->sendFragment("");
    }

    template <typename Response>
    void sendSseComment(const std::shared_ptr<Response>& response, const std::string& comment) {
        response->sendFragment(": " + comment);
    }

    template <typename Response>
    void sendSseDefaultMessageEvent(const std::shared_ptr<Response>& response, const std::string& id, const std::string& data) {
        response->sendFragment("id: " + id);
        response->sendFragment("data: " + data);
        response->sendFragment("");
    }

    template <typename Response>
    void closeSseConnection(const std::shared_ptr<Response>& response) {
        response->getSocketContext()->close();
    }

    template <typename Request, typename Response, typename StateT, typename SendEvents>
    void handleSseRequest(const std::shared_ptr<Request>& request,
                          const std::shared_ptr<Response>& response,
                          StateT& state,
                          const std::vector<ExpectedEvent>& events,
                          bool multiLineData,
                          SendEvents sendEvents) {
        ++state.serverRequestCount;
        state.serverSawPath = request->url == "/events";
        state.serverSawAccept = web::http::ciContains(request->get("Accept"), "text/event-stream");

        response->status(200)
            .type("text/event-stream")
            .set("Cache-Control", "no-cache")
            .set("Connection", state.allowReconnect && state.serverRequestCount == 1 ? "close" : "keep-alive");
        response->sendHeader();

        sendEvents(request, response, state, events);
    }

    template <typename Request, typename Response>
    void handleSseRequest(const std::shared_ptr<Request>& request,
                          const std::shared_ptr<Response>& response,
                          State& state,
                          const std::vector<ExpectedEvent>& events,
                          bool multiLineData = false) {
        handleSseRequest(request, response, state, events, multiLineData, [multiLineData](const auto&, const auto& response, State&, const auto& events) {
            if (multiLineData) {
                sendSseMultiLineDataEvent(response, events.front().type, events.front().id);
            } else {
                for (const ExpectedEvent& event : events) {
                    sendSseEvent(response, event);
                }
            }
        });
    }

    inline void recordEvent(State& state, const web::http::client::tools::EventSource::MessageEvent& message) {
        ++state.clientEventCount;
        state.receivedEvents.push_back(ExpectedEvent{message.type, message.lastEventId, message.data});
    }

    inline void maybeStopAfterExpectedEvents(State& state, std::size_t expectedEventCount) {
        if (state.receivedEvents.size() == expectedEventCount) {
            state.stoppedAfterExpectedEvents = true;
            if (state.eventSource) {
                state.closeInvoked = true;
                state.eventSource->close();
            }
            core::SNodeC::stop();
        }
    }

    template <typename StateT, typename Expect, typename SendEvents, typename AfterFree>
    int runInetEventSourceTest(int argc,
                               char* argv[],
                               const char* testName,
                               const char* serverName,
                               StateT& state,
                               const std::vector<ExpectedEvent>& events,
                               bool multiLineData,
                               Expect expect,
                               SendEvents sendEvents,
                               AfterFree afterFree) {
        int result = tests::support::cTestSkipReturnCode;

        if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
            tests::support::printRootWithoutSNodeCGroupSkipMessage(testName);
        } else {
            tests::support::TestResult testResult;
            core::SNodeC::init(argc, argv);

            using Server = web::http::legacy::in::Server;
            const Server server(serverName, [&state, &events, multiLineData, sendEvents](const auto& request, const auto& response) {
                handleSseRequest(request, response, state, events, multiLineData, sendEvents);
            });

            server.getConfig()->Instance::forceUnrequired();

            server.listen(net::in::SocketAddress("127.0.0.1", 0), [&state, &events](const net::in::SocketAddress& socketAddress, core::socket::State listenState) {
                if (listenState == core::socket::State::OK) {
                    ++state.listenOkCount;
                    const std::uint16_t effectivePort = socketAddress.getPort();
                    if (effectivePort != 0) {
                        ++state.effectiveListenEndpointOkCount;
                        state.eventSource = web::http::legacy::in::EventSource("127.0.0.1", effectivePort, "/events");
                        if (!state.eventSource) {
                            ++state.unexpectedStateCount;
                            core::SNodeC::stop();
                            return;
                        }
                        state.eventSource->onOpen([&state]() { ++state.eventSourceOpenCount; });
                        state.eventSource->onError([&state]() {
                            if (!state.stoppedAfterExpectedEvents && !state.allowReconnect) {
                                ++state.parseErrorCount;
                                core::SNodeC::stop();
                            }
                        });
                        std::set<std::string> registeredEventTypes;
                        for (const ExpectedEvent& event : events) {
                            if (registeredEventTypes.insert(event.type).second) {
                                state.eventSource->addEventListener(event.type, [&state, expectedEventCount = events.size()](const auto& message) {
                                    recordEvent(state, message);
                                    maybeStopAfterExpectedEvents(state, expectedEventCount);
                                });
                            }
                        }
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
            expect(testResult, state, events, startResult);
            state.eventSource.reset();
            core::SNodeC::free();
            afterFree(testResult);
            result = testResult.processResult();
        }

        return result;
    }

    template <typename StateT, typename Expect, typename SendEvents>
    int runInetEventSourceTest(int argc,
                               char* argv[],
                               const char* testName,
                               const char* serverName,
                               StateT& state,
                               const std::vector<ExpectedEvent>& events,
                               bool multiLineData,
                               Expect expect,
                               SendEvents sendEvents) {
        return runInetEventSourceTest(
            argc, argv, testName, serverName, state, events, multiLineData, expect, sendEvents, [](tests::support::TestResult&) {
            });
    }

    template <typename Expect>
    int runInetEventSourceTest(int argc,
                               char* argv[],
                               const char* testName,
                               const char* serverName,
                               State& state,
                               const std::vector<ExpectedEvent>& events,
                               bool multiLineData,
                               Expect expect) {
        return runInetEventSourceTest(
            argc,
            argv,
            testName,
            serverName,
            state,
            events,
            multiLineData,
            expect,
            [multiLineData](const auto&, const auto& response, State&, const auto& events) {
                if (multiLineData) {
                    sendSseMultiLineDataEvent(response, events.front().type, events.front().id);
                } else {
                    for (const ExpectedEvent& event : events) {
                        sendSseEvent(response, event);
                    }
                }
            });
    }

    inline void expectCommon(tests::support::TestResult& testResult, const State& state, int startResult) {
        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv4 EventSource component test");
        testResult.expectEqual(1, state.listenOkCount, "IPv4 SSE server listen callback reports OK exactly once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "IPv4 SSE server reports one effective endpoint");
        testResult.expectEqual(1, state.eventSourceOpenCount, "IPv4 EventSource opens exactly once");
        testResult.expectEqual(1, state.serverRequestCount, "IPv4 SSE server observes exactly one request");
        testResult.expectTrue(state.serverSawPath, "IPv4 SSE server observes /events request path");
        testResult.expectTrue(state.serverSawAccept, "IPv4 SSE server observes Accept: text/event-stream");
        testResult.expectEqual(0, state.parseErrorCount, "IPv4 EventSource reports no parse errors");
        testResult.expectEqual(0, state.unexpectedDisconnectCount, "IPv4 EventSource reports no unexpected disconnects before expected events");
        testResult.expectEqual(0, state.unexpectedStateCount, "IPv4 EventSource reports no unexpected states");
        testResult.expectTrue(state.stoppedAfterExpectedEvents, "IPv4 EventSource stops deterministically after expected events");
    }

    inline void expectEvents(tests::support::TestResult& testResult, const State& state, const std::vector<ExpectedEvent>& expectedEvents) {
        testResult.expectEqual(static_cast<int>(expectedEvents.size()), state.clientEventCount, "IPv4 EventSource receives exact event count");
        testResult.expectEqual(static_cast<int>(expectedEvents.size()), static_cast<int>(state.receivedEvents.size()), "IPv4 EventSource records exact event count");
        const std::size_t comparableCount = std::min(expectedEvents.size(), state.receivedEvents.size());
        for (std::size_t i = 0; i < comparableCount; ++i) {
            testResult.expectTrue(state.receivedEvents[i].type == expectedEvents[i].type, "IPv4 EventSource receives expected event type at index " + std::to_string(i));
            testResult.expectTrue(state.receivedEvents[i].id == expectedEvents[i].id, "IPv4 EventSource receives expected event id at index " + std::to_string(i));
            testResult.expectTrue(state.receivedEvents[i].data == expectedEvents[i].data, "IPv4 EventSource receives expected event data at index " + std::to_string(i));
        }
    }

    struct CloseState : State {};

    inline int runInetEventSourceClientCloseAfterEventTest([[maybe_unused]] int argc, [[maybe_unused]] char* argv[], CloseState& state) {
        tests::support::Phase3SemanticLogCapture capture("snodec-phase3-eventsource-close");
        char testName[] = "InetSseEventSourceClientCloseAfterEventTest";
        char logLevel[] = "--log-level=6";
        char logFormat[] = "--log-format=json";
        char quiet[] = "--quiet";
        char* logArgs[] = {testName, logLevel, logFormat, quiet, nullptr};
        return runInetEventSourceTest(
            4,
            logArgs,
            "InetSseEventSourceClientCloseAfterEventTest",
            "ipv4-sse-eventsource-client-close-after-event-server",
            state,
            {{"measurement", "1", "close-now"}},
            false,
            [](tests::support::TestResult& testResult, const CloseState& observedState, const auto& expectedEvents, int startResult) {
                expectCommon(testResult, observedState, startResult);
                expectEvents(testResult, observedState, expectedEvents);
                testResult.expectEqual(0, startResult, "IPv4 EventSource exits cleanly after client close");
                testResult.expectEqual(1, observedState.serverRequestCount, "IPv4 EventSource receives the close-after-event message");
                testResult.expectTrue(observedState.closeInvoked, "IPv4 EventSource close is invoked by the client after receiving the expected event");
            },
            [](const auto&, const auto& response, CloseState&, const auto&) {
                sendSseEvent(response, {"measurement", "1", "close-now"});
            },
            [&capture](tests::support::TestResult& testResult) {
                const auto records = capture.finish();
                constexpr std::string_view component = "web.http.client.eventsource";
                testResult.expectEqual(1, capture.count(records, component, "", "event source started:"), "EventSource starts once");
                testResult.expectEqual(
                    1, capture.count(records, component, "", "event stream established:"), "event stream establishes once");
                testResult.expectEqual(
                    1, capture.count(records, component, "", "event source stopped:"), "explicit close stops EventSource once");
                testResult.expectEqual(0,
                                       capture.count(records, component, "", "event source reconnect scheduled:"),
                                       "explicit close does not schedule an EventSource reconnect");
                testResult.expectEqual(0,
                                       capture.count(records, component, "", "event source reconnect dispatched:"),
                                       "explicit close does not dispatch an EventSource reconnect");
                testResult.expectTrue(capture.matchingIdentityAndLevel(records, component, "", "event source started:", "info", false),
                                      "EventSource start uses framework connection scope and Info");
                testResult.expectTrue(capture.matchingIdentityAndLevel(records, component, "", "event source stopped:", "info", false),
                                      "EventSource stop uses the same framework connection scope and Info");
                const std::size_t start = capture.position(records, component, "event source started:");
                const std::size_t established = capture.position(records, component, "event stream established:");
                const std::size_t stopped = capture.position(records, component, "event source stopped:");
                testResult.expectTrue(start < established && established < stopped,
                                      "EventSource start, stream establishment, and stop are ordered");
            });
    }

    struct ReconnectState : State {};

    inline int runInetEventSourceReconnectLifecycleTest([[maybe_unused]] int argc, [[maybe_unused]] char* argv[], ReconnectState& state) {
        state.allowReconnect = true;
        tests::support::Phase3SemanticLogCapture capture("snodec-phase3-eventsource-reconnect");
        char testName[] = "InetSseEventSourceReconnectLifecycleTest";
        char logLevel[] = "--log-level=6";
        char logFormat[] = "--log-format=json";
        char quiet[] = "--quiet";
        char* logArgs[] = {testName, logLevel, logFormat, quiet, nullptr};
        return runInetEventSourceTest(
            4,
            logArgs,
            "InetSseEventSourceReconnectLifecycleTest",
            "ipv4-sse-eventsource-reconnect-lifecycle-server",
            state,
            {{"measurement", "2", "reconnected"}},
            false,
            [](tests::support::TestResult& testResult, const ReconnectState& observedState, const auto& expectedEvents, int startResult) {
                testResult.expectEqual(0, startResult, "EventSource reconnect lifecycle exits cleanly");
                testResult.expectEqual(1, observedState.listenOkCount, "EventSource reconnect server listens once");
                testResult.expectEqual(1, observedState.eventSourceOpenCount, "EventSource stream establishes after retry");
                testResult.expectEqual(2, observedState.serverRequestCount, "EventSource retry dispatches a second HTTP request");
                testResult.expectEqual(0, observedState.parseErrorCount, "EventSource reconnect has no parse error");
                testResult.expectEqual(0, observedState.unexpectedStateCount, "EventSource reconnect has no unexpected state");
                expectEvents(testResult, observedState, expectedEvents);
                testResult.expectTrue(observedState.closeInvoked, "EventSource closes explicitly after the reconnected event");
            },
            [](const auto&, const auto& response, ReconnectState& observedState, const auto&) {
                if (observedState.serverRequestCount == 1) {
                    response->sendFragment("retry: 10");
                    response->sendFragment("");
                    response->end();
                } else {
                    sendSseEvent(response, {"measurement", "2", "reconnected"});
                }
            },
            [&capture](tests::support::TestResult& testResult) {
                const auto records = capture.finish();
                constexpr std::string_view component = "web.http.client.eventsource";
                testResult.expectEqual(
                    1, capture.count(records, component, "", "event source started:"), "reconnecting EventSource starts once");
                testResult.expectEqual(
                    1, capture.count(records, component, "", "event stream established:"), "EventSource records the established stream");
                testResult.expectEqual(1,
                                       capture.count(records, component, "", "event source reconnect scheduled:"),
                                       "EventSource schedules one protocol retry");
                testResult.expectEqual(1,
                                       capture.count(records, component, "", "event source reconnect dispatched:"),
                                       "EventSource dispatches one protocol retry");
                testResult.expectEqual(
                    1, capture.count(records, component, "", "event source stopped:"), "reconnecting EventSource stops once");
                testResult.expectTrue(capture.matchingIdentityAndLevel(records, component, "", "event source reconnect ", "debug", false),
                                      "EventSource retry lifecycle uses framework connection scope and Debug");
                const std::size_t scheduled = capture.position(records, component, "event source reconnect scheduled:");
                const std::size_t dispatched = capture.position(records, component, "event source reconnect dispatched:");
                const std::size_t stopped = capture.position(records, component, "event source stopped:");
                testResult.expectTrue(scheduled < dispatched && dispatched < stopped,
                                      "EventSource retry schedule, dispatch, and stop are ordered");
            });
    }

} // namespace tests::component::eventsource

#endif // TESTS_COMPONENT_EVENTSOURCE_EVENTSOURCECOMPONENTTEST_H
