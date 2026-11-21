#include <core/socket/stream/SocketClient.h>
#include <core/socket/stream/SocketServer.h>
#include <express/legacy/in/Server.h>
#include <mcp/server/Server.h>
#include <mcp/server/Tool.h>
#include <mcp/server/http/Router.h>
#include <mcp/server/stdio/Transport.h>
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/un/stream/legacy/SocketClient.h>

namespace {
    class InstalledTool final : public mcp::server::Tool {
    public:
        InstalledTool();
        void call(const mcp::server::ToolCall&, const std::shared_ptr<mcp::server::ToolReply>& reply) override;
    };
    InstalledTool::InstalledTool()
        : Tool({"installed", {}, mcp::server::Json::parse(R"json(
{
    "type": "object"
}
)json")}) {
    }
    void InstalledTool::call(const mcp::server::ToolCall&, const std::shared_ptr<mcp::server::ToolReply>& reply) {
        reply->result(mcp::server::ToolResult::text("installed consumer"));
    }
} // namespace
int main() {
    const auto run = &mcp::server::stdio::Transport::run;
    static_cast<void>(run);
    static_assert(sizeof(mcp::server::http::Router) >= sizeof(express::Router));
    mcp::server::Server server("installed-consumer", "test-version");
    server.tool(std::make_shared<InstalledTool>());
    return 0;
}
