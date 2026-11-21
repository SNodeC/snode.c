#include <core/socket/stream/SocketClient.h>
#include <core/socket/stream/SocketServer.h>
#include <express/legacy/in/Server.h>
#include <mcp/server/Server.h>
#include <mcp/server/http/Router.h>
#include <mcp/server/stdio/Transport.h>
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/un/stream/legacy/SocketClient.h>

int main() {
    const auto retainProtocol = &mcp::server::stdio::Transport::retainProtocol;
    static_cast<void>(retainProtocol);
    static_assert(sizeof(mcp::server::http::Router) >= sizeof(express::Router));
    mcp::server::Server server("installed-consumer");
    return 0;
}
