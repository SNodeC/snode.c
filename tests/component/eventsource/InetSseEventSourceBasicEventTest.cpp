#include "EventSourceComponentTest.h"

int main(int argc, char* argv[]) {
    tests::component::eventsource::State state;
    const std::vector<tests::component::eventsource::ExpectedEvent> events{{"measurement", "1", "temperature=21.5"}};

    return tests::component::eventsource::runInetEventSourceTest(
        argc,
        argv,
        "InetSseEventSourceBasicEventTest",
        "ipv4-sse-eventsource-basic-event-server",
        state,
        events,
        false,
        [](tests::support::TestResult& testResult, const auto& observedState, const auto& expectedEvents, int startResult) {
            tests::component::eventsource::expectCommon(testResult, observedState, startResult);
            tests::component::eventsource::expectEvents(testResult, observedState, expectedEvents);
        });
}
