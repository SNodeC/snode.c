#include "EventSourceComponentTest.h"

int main(int argc, char* argv[]) {
    tests::component::eventsource::State state;
    const std::vector<tests::component::eventsource::ExpectedEvent> events{{"measurement", "1", "retry-updated"}};

    return tests::component::eventsource::runInetEventSourceTest(
        argc,
        argv,
        "InetSseEventSourceRetryFieldTest",
        "ipv4-sse-eventsource-retry-field-server",
        state,
        events,
        false,
        [](tests::support::TestResult& testResult,
           const tests::component::eventsource::State& observedState,
           const std::vector<tests::component::eventsource::ExpectedEvent>& expectedEvents,
           int startResult) {
            tests::component::eventsource::expectCommon(testResult, observedState, startResult);
            tests::component::eventsource::expectEvents(testResult, observedState, expectedEvents);
            testResult.expectEqual(50, static_cast<int>(observedState.eventSource->retry()), "IPv4 EventSource exposes parsed retry field");
        },
        [](const auto&, const auto& response, tests::component::eventsource::State&, const auto&) {
            response->sendFragment("retry: 50");
            tests::component::eventsource::sendSseEvent(response, {"measurement", "1", "retry-updated"});
        });
}
