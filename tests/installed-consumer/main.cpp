#include <core/socket/stream/SocketClient.h>
#include <core/socket/stream/SocketServer.h>
#include <express/legacy/in/Server.h>
#include <mcp/server/Server.h>
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/un/stream/legacy/SocketClient.h>

int main() {
    mcp::server::Server server("installed-consumer");
    return server.valid() ? 0 : 1;
}
