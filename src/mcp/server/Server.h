// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_SERVER_H
#define MCP_SERVER_SERVER_H

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

namespace mcp::server {
    using Json = nlohmann::json;
    inline constexpr const char* modernVersion = "2026-07-28";
    inline constexpr const char* legacyVersion = "2025-11-25";
    inline constexpr const char* versionKey = "io.modelcontextprotocol/protocolVersion";
    inline constexpr const char* capabilitiesKey = "io.modelcontextprotocol/clientCapabilities";
    inline constexpr const char* subscriptionKey = "io.modelcontextprotocol/subscriptionId";

    class Reply {
    public:
        using Output = std::function<void(const Json&, bool)>;
        Reply(Json id, bool modern, bool cacheable, Output output, std::chrono::milliseconds timeout);
        void result(Json value);
        void error(int code, const std::string& message, Json data = nullptr);
        void notify(const std::string& method, Json params);
        void cancel(bool serverInitiated = false);
        bool active() const;
        bool expired() const;
        void expire();
        void listen();
        bool isSubscription() const;

    private:
        friend class Server;
        Json outputSchema;
        Json id;
        bool modern;
        bool cacheable;
        bool toolRequest = false;
        bool done = false;
        bool subscription = false;
        Output output;
        std::chrono::steady_clock::time_point deadline;
    };

    class Server {
    public:
        using Handler = std::function<void(const Json&, const std::shared_ptr<Reply>&)>;
        explicit Server(std::string name);
        void tool(Json description, Handler handler);
        void resource(Json description, Handler handler);
        void prompt(Json description, Handler handler);
        bool valid() const;
        std::shared_ptr<Reply> dispatch(const Json& request,
                                        bool legacy,
                                        Reply::Output output,
                                        std::chrono::milliseconds timeout = std::chrono::seconds(30)) const;
        static bool checkSchema(const Json& schema, std::string& error);
        static bool validate(const Json& schema, const Json& value, std::string& error);
        static bool validRequest(const Json& request);
        static int httpStatus(const Json& message);

    private:
        enum class Kind { Tool, Resource, Prompt };
        struct Entry {
            Kind kind;
            Json description;
            Handler handler;
        };
        void add(Kind kind, Json description, Handler handler);
        Json capabilities() const;
        std::string name;
        bool registrationsValid = true;
        std::map<std::string, Entry> entries;
    };
} // namespace mcp::server
#endif
