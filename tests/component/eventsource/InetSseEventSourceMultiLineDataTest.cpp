#include "EventSourceComponentTest.h"

int main(int argc, char* argv[]) {
    tests::component::eventsource::State state;
    const std::vector<tests::component::eventsource::ExpectedEvent> events{{"measurement", "1", "line-1\nline-2\nline-3"}};

    return tests::component::eventsource::runInetEventSourceTest(
        argc,
        argv,
        "InetSseEventSourceMultiLineDataTest",
        "ipv4-sse-eventsource-multiline-data-server",
        state,
        events,
        true,
        [](tests::support::TestResult& testResult, const auto& observedState, const auto& expectedEvents, int startResult) {
            tests::component::eventsource::expectCommon(testResult, observedState, startResult);
            tests::component::eventsource::expectEvents(testResult, observedState, expectedEvents);
        });
}
