#include <core/SNodeC.h>
#include <core/socket/stream/SocketClient.h>
#include <core/socket/stream/SocketContext.h>
#include <core/socket/stream/SocketContextFactory.h>
#include <core/socket/stream/SocketServer.h>
#include <express/legacy/in/Server.h>
#include <mcp/server/Server.h>
#include <mcp/server/Tool.h>
#include <mcp/server/http/Router.h>
#include <mcp/server/stream/SocketContextFactory.h>
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/stdio/stream/SocketEndpoint.h>
#include <net/un/stream/legacy/SocketClient.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>

namespace {
    class InstalledContext final : public core::socket::stream::SocketContext {
    public:
        explicit InstalledContext(core::socket::stream::SocketConnection* connection)
            : SocketContext(connection) {
        }

    private:
        void onConnected() override {
            close();
        }
        void onDisconnected() override {
        }
        std::size_t onReceivedFromPeer() override {
            char value;
            return readFromPeer(&value, 1);
        }
        bool onSignal([[maybe_unused]] int signal) override {
            return true;
        }
    };
    class InstalledFactory final : public core::socket::stream::SocketContextFactory {
    public:
        ~InstalledFactory() override;
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) override {
            return new InstalledContext(connection);
        }
    };
    InstalledFactory::~InstalledFactory() = default;
    class InstalledTool final : public mcp::server::Tool {
    public:
        InstalledTool();
        void call(const mcp::server::ToolCall&, const std::shared_ptr<mcp::server::ToolReply>& reply) override;
    };
    InstalledTool::InstalledTool()
        : Tool({"installed", {}, nlohmann::ordered_json::parse(R"json(
{
    "type": "object"
}
)json")}) {
    }
    void InstalledTool::call(const mcp::server::ToolCall&, const std::shared_ptr<mcp::server::ToolReply>& reply) {
        reply->result(mcp::server::ToolResult::text("installed consumer"));
    }
} // namespace
int main([[maybe_unused]] int argc, char* argv[]) {
    int result = 1;
    int pair[2]{};
    static_assert(std::is_base_of_v<core::socket::stream::SocketContextFactory, mcp::server::stream::SocketContextFactory>);
    static_assert(sizeof(mcp::server::http::Router) >= sizeof(express::Router));
    mcp::server::Server server("installed-consumer", "test-version");
    server.tool(std::make_shared<InstalledTool>());
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0) {
        const net::stdio::stream::SocketEndpoint<InstalledFactory> endpoint("installed-endpoint");
        endpoint.getConfig()->Instance::forceUnrequired();
        char file[] = "--config-file=/dev/null", quiet[] = "--quiet";
        char* arguments[] = {argv[0], file, quiet};
        core::SNodeC::init(3, arguments);
        int attached = 0;
        endpoint.attach(net::stdio::SocketAddress(pair[0], pair[0]), [&attached](const auto&, auto state) {
            if (state == core::socket::State::OK) {
                ++attached;
            }
        });
        const int loop = core::SNodeC::start();
        ::close(pair[1]);
        result = loop == 0 && attached == 1 ? 0 : 1;
    }
    return result;
}
