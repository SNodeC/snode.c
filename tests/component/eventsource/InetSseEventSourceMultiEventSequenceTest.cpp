#include "EventSourceComponentTest.h"

int main(int argc, char* argv[]) {
    tests::component::eventsource::State state;
    const std::vector<tests::component::eventsource::ExpectedEvent> events{
        {"measurement", "1", "value-1"},
        {"measurement", "2", "value-2"},
        {"status", "3", "done"},
    };

    return tests::component::eventsource::runInetEventSourceTest(
        argc,
        argv,
        "InetSseEventSourceMultiEventSequenceTest",
        "ipv4-sse-eventsource-multi-event-sequence-server",
        state,
        events,
        false,
        [](tests::support::TestResult& testResult, const auto& observedState, const auto& expectedEvents, int startResult) {
            tests::component::eventsource::expectCommon(testResult, observedState, startResult);
            tests::component::eventsource::expectEvents(testResult, observedState, expectedEvents);
        });
}
