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
    const auto run = &mcp::server::stdio::Transport::run;
    static_cast<void>(run);
    static_assert(sizeof(mcp::server::http::Router) >= sizeof(express::Router));
    mcp::server::Server server("installed-consumer", "test-version");
    return 0;
}
