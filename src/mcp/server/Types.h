// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_TYPES_H
#define MCP_SERVER_TYPES_H
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>
namespace mcp::server {
    /// Owned JSON value using the library's configured JSON implementation.
    using Json = nlohmann::json;
    /// Authenticated application identity; never inferred from request arguments.
    struct Principal {
        std::string subject; ///< Owned identifier assigned by the authorization callback.
        Json claims;         ///< Owned application claims, unavailable to unauthenticated callers.
    };
    /// Preserve a string or signed integer token's JSON type without parsing _meta.
    using ProgressToken = std::variant<std::string, std::int64_t>;
    /// Terminal cancellation cause delivered on the event-loop thread.
    enum class CancelReason {
        Client,       ///< Client explicitly cancelled the request.
        Deadline,     ///< The configured request deadline elapsed.
        Disconnected, ///< The transport lost its caller.
        Shutdown      ///< The transport is shutting down.
    };
    /// Whether a once-only completion was accepted on the event-loop thread.
    enum class Completion {
        Accepted, ///< This call completed the live request.
        Inactive  ///< The request had already completed or been cancelled.
    };
    /// Application errors valid in both protocol eras; envelope errors stay private.
    enum class ErrorCode : int {
        InvalidParams = -32602, ///< The supplied application parameters are invalid.
        InternalError = -32603  ///< Application processing failed internally.
    };
    /// Sender or intended audience of a content block.
    enum class Role {
        User,     ///< Content from or intended for the user.
        Assistant ///< Content from or intended for the assistant.
    };
    /// Owned optional content annotations; no authorization semantics.
    struct Annotations {
        std::vector<Role> audience;              ///< Empty omits the audience annotation.
        std::optional<double> priority;          ///< Optional priority in the protocol's [0,1] range.
        std::optional<std::string> lastModified; ///< Optional ISO 8601 timestamp.
    };
    /// Owned progress update; send only on the event-loop thread.
    struct Progress {
        double value;                ///< Monotonically increasing completed work.
        std::optional<double> total; ///< Optional expected total work.
        std::string message;         ///< Optional human-readable status; empty omits it.
    };
} // namespace mcp::server
#endif
