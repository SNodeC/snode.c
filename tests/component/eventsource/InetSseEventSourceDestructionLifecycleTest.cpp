#include "EventSourceComponentTest.h"

int main(int argc, char* argv[]) {
    tests::component::eventsource::DestructionState state;
    return tests::component::eventsource::runInetEventSourceDestructionLifecycleTest(argc, argv, state);
}
