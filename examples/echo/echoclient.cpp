#include "EchoSocketContext.h"

#include <core/SNodeC.h>
#include <core/socket/State.h>
#include <net/in/stream/legacy/SocketClient.h>

using EchoSocketClient = net::in::stream::legacy::SocketClient<echo::EchoClientSocketContextFactory>;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const EchoSocketClient client("echoclient");

    client.connect(
        [log = client.log()](const EchoSocketClient::SocketAddress& socketAddress, core::socket::State state) {
            if (state == core::socket::State::OK) {
                log.info("Connected to {}", socketAddress.toString());
            }
        });

    return core::SNodeC::start();
}
