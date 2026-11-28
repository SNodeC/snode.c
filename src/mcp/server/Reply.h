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

#ifndef MCP_SERVER_REPLY_H
#define MCP_SERVER_REPLY_H

#include "mcp/ClientInputs.h" // IWYU pragma: export
#include "mcp/server/Results.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <optional>
#include <string>
#include <type_traits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    namespace detail {
        class Access;    ///< Module-private construction access.
        struct Response; ///< Shared opaque request and completion state.
    } // namespace detail

    /// Share the handle for asynchronous work; construction is module-private.
    template <class Result>
    class Reply {
    public:
        Reply(const Reply&) = delete;            ///< Non-transferable handle; share ownership through std::shared_ptr instead.
        Reply& operator=(const Reply&) = delete; ///< Non-transferable handle; share ownership through std::shared_ptr instead.
        Reply(Reply&&) = delete;                 ///< Non-transferable handle; share ownership through std::shared_ptr instead.
        Reply& operator=(Reply&&) = delete;      ///< Non-transferable handle; share ownership through std::shared_ptr instead.

        /// Complete once with the matching result kind on the event-loop thread.
        Completion result(Result result);
        /// Complete once with an application error on the event-loop thread.
        Completion error(ErrorCode code, std::string message, Json data = nullptr);
        /// Request input using the negotiated era and advertised client modes.
        /// Modern: complete with input_required. Legacy: resume the same handler after nested RPC responses.
        /// Accepted means input was requested; legacy callers must await the resumed handler, not complete again.
        Completion inputRequired(InputRequired input)
            requires(!std::is_same_v<Result, CompletionResult>);
        /// Mixed elicitation/sampling/roots round; retains the existing request lifetime.
        Completion clientInputRequired(ClientInputRequired input)
            requires(!std::is_same_v<Result, CompletionResult>);
        /// Legacy reverse requests, including client-owned task lifecycle and ping.
        /// Low-level untyped escape hatch; prefer clientInputRequired for typed client inputs.
        /// Uses the same correlated input round and resumption as clientInputRequired.
        Completion clientRequestsRequired(Json requests, std::optional<std::string> requestState = {})
            requires(!std::is_same_v<Result, CompletionResult>);
        /// Send progress on the event loop with the original typed token; no token is a no-op.
        void progress(Progress progress);
        /// Update a live task's working/input-required status and human-readable message.
        bool taskStatus(TaskState state, const std::string& message = {})
            requires(std::is_same_v<Result, ToolResult>);
        /// Materialize a negotiated modern task while retaining this asynchronous reply.
        /// Legacy augmented calls are materialized before the handler runs.
        Completion startTask(TaskOptions options = {})
            requires(std::is_same_v<Result, ToolResult>);

    private:
        explicit Reply(std::shared_ptr<detail::Response> impl);

        std::shared_ptr<detail::Response> impl;

        friend class detail::Access;
    };

    /// Completion handle instantiated by the library; event-loop lifetime and call-once rules apply.
    using CompletionReply = Reply<CompletionResult>;
    /// Instantiation supplied by the MCP library.
    extern template class Reply<CompletionResult>;

    /// Typed tool completion handle; ownership is shared by asynchronous handlers.
    using ToolReply = Reply<ToolResult>;
    /// Typed resource completion handle; ownership is shared by asynchronous handlers.
    using ResourceReply = Reply<ResourceResult>;
    /// Typed prompt completion handle; ownership is shared by asynchronous handlers.
    using PromptReply = Reply<PromptResult>;
    /// Instantiations are supplied by the MCP library; no consumer implementation.
    extern template class Reply<ToolResult>;
    /// Instantiation supplied by the MCP library.
    extern template class Reply<ResourceResult>;
    /// Instantiation supplied by the MCP library.
    extern template class Reply<PromptResult>;

} // namespace mcp::server

#endif
