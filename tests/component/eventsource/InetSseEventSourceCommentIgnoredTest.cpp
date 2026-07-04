#include "EventSourceComponentTest.h"

int main(int argc, char* argv[]) {
    tests::component::eventsource::State state;
    const std::vector<tests::component::eventsource::ExpectedEvent> events{{"measurement", "1", "value-1"}, {"status", "2", "done"}};

    return tests::component::eventsource::runInetEventSourceTest(
        argc,
        argv,
        "InetSseEventSourceCommentIgnoredTest",
        "ipv4-sse-eventsource-comment-ignored-server",
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
            tests::component::eventsource::sendSseComment(response, "comment-before");
            tests::component::eventsource::sendSseEvent(response, {"measurement", "1", "value-1"});
            tests::component::eventsource::sendSseComment(response, "comment-between");
            tests::component::eventsource::sendSseEvent(response, {"status", "2", "done"});
        });
}
