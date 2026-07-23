#include "EventSourceComponentTest.h"

int main(int argc, char* argv[]) {
    tests::component::eventsource::ReconnectState state;
    return tests::component::eventsource::runInetEventSourceReconnectLifecycleTest(argc, argv, state);
}
