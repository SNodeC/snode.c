#include "EventSourceComponentTest.h"

int main(int argc, char* argv[]) {
    tests::component::eventsource::State state;
    const std::vector<tests::component::eventsource::ExpectedEvent> events{{"message", "1", "default-message"}};

    return tests::component::eventsource::runInetEventSourceTest(
        argc,
        argv,
        "InetSseEventSourceDefaultMessageEventTest",
        "ipv4-sse-eventsource-default-message-event-server",
        state,
        events,
        false,
        [](tests::support::TestResult& testResult,
           const tests::component::eventsource::State& observedState,
           const std::vector<tests::component::eventsource::ExpectedEvent>& expectedEvents,
           int startResult) {
            tests::component::eventsource::expectCommon(testResult, observedState, startResult);
            tests::component::eventsource::expectEvents(testResult, observedState, expectedEvents);
        },
        [](const auto&, const auto& response, tests::component::eventsource::State&, const auto&) {
            tests::component::eventsource::sendSseDefaultMessageEvent(response, "1", "default-message");
        });
}
