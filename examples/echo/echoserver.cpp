#include "EchoSocketContext.h"

#include <core/SNodeC.h>
#include <core/socket/State.h>
#include <net/in/stream/legacy/SocketServer.h>

using EchoSocketServer = net::in::stream::legacy::SocketServer<echo::EchoServerSocketContextFactory>;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const EchoSocketServer server("echoserver");

    server.listen(
        [log = server.log()](const EchoSocketServer::SocketAddress& socketAddress, core::socket::State state) {
            if (state == core::socket::State::OK) {
                log.info("Listening on {}", socketAddress.toString());
            }
        });

    return core::SNodeC::start();
}
