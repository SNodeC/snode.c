/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MCP_TYPES_H
#define MCP_TYPES_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp {

    /// Owned JSON value using the library's configured JSON implementation.
    using Json = nlohmann::json;

    /// RFC 5424 severities in increasing importance; protocol logging is legacy-only.
    enum class LogLevel { Debug, Info, Notice, Warning, Error, Critical, Alert, Emergency };
    std::string loggingLevelName(LogLevel level);
    std::optional<LogLevel> parseLoggingLevel(const std::string& name);

    struct LogMessage {
        LogLevel level;
        Json data;
        std::string logger;
    };

    /// Tool execution policy; task support is opt-in on each registration.
    enum class TaskSupport { Forbidden, Optional, Required };

    struct TaskOptions {
        std::uint64_t ttlMs = 60000;
        std::uint64_t pollIntervalMs = 1000;
    };

    enum class TaskState { Working, InputRequired, Completed, Failed, Cancelled };

    struct TaskInfo {
        std::string taskId;
        TaskState status;
        std::string createdAt;
        std::string lastUpdatedAt;
        std::optional<std::uint64_t> ttlMs;
        std::uint64_t pollIntervalMs = 1000;
        Json result;
        Json error;
        Json inputRequests;
    };

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
        Accepted, ///< This call completed the request or accepted an input request.
        Inactive  ///< The request had already completed or been cancelled.
    };
    /// Application errors valid in both protocol eras; envelope errors stay private.
    enum class ErrorCode : int {
        InvalidParams = -32602,             ///< The supplied application parameters are invalid.
        InternalError = -32603,             ///< Application processing failed internally.
        ResourceNotFound = -32002,          ///< Legacy resource lookup failure.
        HeaderMismatch = -32020,            ///< Modern HTTP headers disagree with the envelope.
        MissingClientCapability = -32021,   ///< A request requires an unadvertised capability.
        UnsupportedProtocolVersion = -32022 ///< Modern revision is unsupported.
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

    /// User's explicit elicitation disposition; never an authorization assertion.
    enum class ElicitationAction { Accept, Decline, Cancel };

    /// Owned elicitation input; content must be validated by the application against the requested form.
    struct InputResponse {
        ElicitationAction action; ///< Explicit client choice, delivered on the event loop.
        Json content{};           ///< Owned untrusted form data; null when absent.
    };

    /// Form elicitation for non-sensitive data only; raw schema uses MCP's flat primitive dialect.
    struct FormInput {
        std::string message;  ///< Owned user-facing explanation; never request credentials here.
        Json requestedSchema; ///< Owned flat object schema as specified by elicitation/create.
    };

    /// URL elicitation; application must bind completion to the verified initiating principal.
    struct UrlInput {
        std::string message; ///< Owned user-facing explanation.
        std::string url;     ///< Owned absolute HTTP(S) URL; never fetched by this module.
    };

    /// Owned elicitation round; modern retry or legacy handler resumption, at most 16 requests.
    struct InputRequired {
        std::map<std::string, std::variant<FormInput, UrlInput>> requests; ///< Owned elicitation requests.
        std::optional<std::string>
            requestState{}; ///< Opaque state echoed untrusted on retry; protect integrity if it affects access or business logic.
    };

    /// Preferred icon background; no rendering or fetching is performed by the server.
    enum class IconTheme { Light, Dark };

    /// Owned display icon; consumers treat its URI/content as untrusted.
    struct Icon {
        std::string src;                       ///< Owned icon URI, normally HTTPS or data.
        std::optional<std::string> mimeType{}; ///< Optional advisory MIME type.
        std::vector<std::string> sizes{};      ///< Optional dimensions or "any".
        std::optional<IconTheme> theme{};      ///< Optional preferred background.
    };

    /// Owned implementation metadata, copied by Server before transport attachment.
    struct Implementation {
        std::string name;                         ///< Owned programmatic name.
        std::string version;                      ///< Owned implementation version.
        std::optional<std::string> title{};       ///< Optional owned display title.
        std::optional<std::string> description{}; ///< Optional human-readable description.
        std::optional<std::string> websiteUrl{};  ///< Optional website URI.
        std::vector<Icon> icons{};                ///< Optional display icons; never fetched.
    };

    /// Cache authorization boundary for modern cacheable results.
    enum class CacheScope { Private, Public };

    /// Owned modern cache hint; private and immediately stale by default.
    struct CacheHints {
        std::uint64_t ttlMs = 0;                ///< Freshness interval in milliseconds; never negative.
        CacheScope scope = CacheScope::Private; ///< Public permits cross-authorization reuse; choose deliberately.
    };

    /// Owned progress update; send only on the event-loop thread.
    struct Progress {
        double value;                ///< Monotonically increasing completed work.
        std::optional<double> total; ///< Optional expected total work.
        std::string message;         ///< Optional human-readable status; empty omits it.
    };

} // namespace mcp

#endif
