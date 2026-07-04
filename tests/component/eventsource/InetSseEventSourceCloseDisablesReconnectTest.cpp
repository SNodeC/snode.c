#include "EventSourceComponentTest.h"

int main(int argc, char* argv[]) {
    tests::component::eventsource::CloseState state;
    return tests::component::eventsource::runInetEventSourceCloseDisablesReconnectTest(argc, argv, state);
}
