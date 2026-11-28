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

#ifndef MCP_SERVER_REQUEST_H
#define MCP_SERVER_REQUEST_H

#include "mcp/ClientInputs.h" // IWYU pragma: export
#include "mcp/Protocol.h"
#include "mcp/server/Types.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    namespace detail {
        class Access;    ///< Module-private construction access.
        struct Response; ///< Shared opaque request state.
    } // namespace detail

    /// Copyable request handle; all access and final release occur on the event loop.
    class RequestContext {
    public:
        /// Once-only observer on cancellation; never called after successful completion.
        using OnCancel = std::function<void(CancelReason)>;

        ProtocolEra getProtocolEra() const;
        /// Borrow the authenticated identity while this context remains alive.
        const std::optional<Principal>& getPrincipal() const;
        /// Borrow the original typed progress token while this context remains alive.
        const std::optional<ProgressToken>& getProgressToken() const;
        /// Borrow untrusted elicitation responses until the next input round; event-loop access only.
        const std::map<std::string, InputResponse>& getInputResponses() const;
        /// Borrow one response; validate accepted content against the server-owned form schema.
        /// Throws invalid_argument on schema/content failure; null means the key is absent.
        const InputResponse* getInputResponse(const std::string& key, const Json& requestedSchema) const;
        /// Validate a sampled response against the original request before returning owned data.
        std::optional<SamplingResult> getSamplingResponse(const std::string& key, const SamplingRequest& request) const;
        std::optional<RootsResult> getRootsResponse(const std::string& key) const;
        /// Request legacy reverse progress for a keyed input; the parent request owns its observation.
        bool trackClientProgress(const std::string& key, std::function<void(const Progress&)> observer = {}) const;
        std::optional<Progress> getClientProgress(const std::string& key) const;
        /// Borrow a validated reverse-request result until the next input round.
        const Json* getClientResponse(const std::string& key) const;
        /// The URL elicitation identifier generated for the most recent input round.
        std::optional<std::string> getElicitationId(const std::string& key) const;
        bool notifyElicitationComplete(const std::string& elicitationId) const;
        /// Replace this legacy session's roots-change observer; cleared on session retirement.
        /// Modern roots are obtained afresh through per-request input continuations.
        bool onRootsChanged(std::function<void()> callback) const;
        /// Borrow opaque untrusted state; verify integrity before using it for access/business decisions.
        const std::optional<std::string>& getRequestState() const;
        /// Register on the event loop; late observers run immediately after cancellation.
        /// Observer exceptions are contained and logged; captures live until termination.
        void onCancel(OnCancel callback) const;
        /// Emit with shared threshold/rate/size limits; modern requests must opt in through metadata.
        bool log(LogLevel level, Json data, const std::string& logger = {}) const;
        std::optional<std::string> getTaskId() const;

    private:
        explicit RequestContext(std::shared_ptr<detail::Response> impl);

        std::shared_ptr<detail::Response> impl;

        friend class detail::Access;
    };

    /// Owned completion input; all strings and context live for this value's lifetime.
    struct CompletionRequest {
        RequestContext context;                         ///< Shared event-loop request context.
        std::string name;                               ///< Argument or template variable being completed.
        std::string value;                              ///< Current partial input, not a fully validated argument.
        std::map<std::string, std::string> arguments{}; ///< Already-resolved contextual arguments.
    };

    /// Owned tool input; arguments have passed the registered schema.
    struct ToolCall {
        RequestContext context; ///< Shared event-loop request context.
        Json arguments;         ///< Owned validated argument object.
    };

    /// Owned resource input, independent of the protocol envelope.
    struct ResourceRead {
        RequestContext context;                         ///< Shared event-loop request context.
        std::string uri;                                ///< Owned requested resource URI.
        std::map<std::string, std::string> variables{}; ///< Owned decoded template variables; empty for exact resources.
    };

    /// Owned prompt input; all argument values are protocol strings.
    struct PromptGet {
        RequestContext context;                       ///< Shared event-loop request context.
        std::map<std::string, std::string> arguments; ///< Owned named string arguments.
    };

} // namespace mcp::server

#endif
