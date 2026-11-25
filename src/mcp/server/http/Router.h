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

#ifndef MCP_SERVER_HTTP_ROUTER_H
#define MCP_SERVER_HTTP_ROUTER_H

#include "express/Router.h"
#include "mcp/Types.h"
#include "mcp/server/Types.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <optional>
#include <string>
#include <variant>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    class Server; ///< Registry borrowed through transport drain.
    class Config; ///< Configuration borrowed through transport drain.

    namespace http {

        /// Accepted request; an absent principal deliberately permits anonymous access.
        struct Authorized {
            std::optional<Principal> principal; ///< Owned authenticated identity, copied into the request context.
        };

        /// Authentication required; emit 401 with a header-safe challenge or configured resource metadata.
        struct Unauthorized {
            std::string wwwAuthenticate;        ///< Owned WWW-Authenticate value; CR/LF and control bytes are rejected.
            std::optional<std::string> scope{}; ///< Optional scopes for this operation; encoded into the Bearer challenge.
        };

        struct Forbidden {}; ///< Deliberate 403 denial without an authentication challenge.

        /// A valid token lacks the scope needed for this operation; emit the RFC 6750 challenge.
        struct InsufficientScope {
            std::string scope; ///< Owned space-delimited required scopes, validated as header-safe.
        };

        using Authorization =
            std::variant<Authorized, Unauthorized, Forbidden, InsufficientScope>; ///< Owned admission decision for one HTTP request.

        /// Legacy notification-channel policy; modern requests never use legacy sessions.
        enum class LegacySubscriptions { Disabled, Enabled };

        /// OAuth protected-resource discovery; token validation remains the application's responsibility.
        struct ProtectedResource {
            std::string resource;                          ///< Canonical absolute resource URI; no fragment, userinfo or query.
            std::vector<std::string> authorizationServers; ///< At least one HTTPS issuer URI.
            std::vector<std::string> scopesSupported{};    ///< Optional advertised scope names.
            std::optional<std::string> resourceName{};     ///< Optional user-facing name.
            std::optional<std::string> documentation{};    ///< Optional documentation URI.
        };

        /// Owned HTTP binding policy, copied into the router at construction.
        struct RouterOptions {
            std::optional<ProtectedResource> protectedResource{};                   ///< Optional RFC 9728 metadata/challenge configuration.
            LegacySubscriptions legacySubscriptions = LegacySubscriptions::Enabled; ///< Enable bounded 2025 session/GET channels.
        };

        /// Express router; copy/mount it while the borrowed registry and config remain alive.
        class Router : public express::Router {
        public:
            /// Runs on the event loop after Origin validation; exceptions become 500 without credential logging.
            using Authorize = std::function<Authorization(const std::shared_ptr<express::Request>&)>;

            /// Attach on the event loop after init; enables runtime registration notifications; empty authorize permits anonymous access.
            /// Retain server and config until this router and its operations have drained.
            Router(Server& server, Config& config, Authorize authorize = {}, RouterOptions options = {});

            /// Mount public discovery routes on the application's root router, outside the MCP endpoint.
            /// Call on the event loop before listen; copies immutable metadata into route handlers.
            const Router& mountMetadata(const express::Router& root) const;

        private:
            std::optional<ProtectedResource> protectedResource;
        };

    } // namespace http

} // namespace mcp::server

#endif
