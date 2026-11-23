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
#include "mcp/server/Types.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <optional>
#include <string>
#include <variant>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    class Server; ///< Registry borrowed through transport drain.
    class Config; ///< Configuration borrowed through transport drain.

    namespace http {

        /// Accepted request; an absent principal deliberately permits anonymous access.
        struct Authorized {
            std::optional<Principal> principal; ///< Owned authenticated identity, copied into the request context.
        };

        /// Authentication required; emit 401 with the supplied nonempty, header-safe challenge.
        struct Unauthorized {
            std::string wwwAuthenticate; ///< Owned WWW-Authenticate value; CR/LF and control bytes are rejected.
        };

        struct Forbidden {}; ///< Deliberate 403 denial without an authentication challenge.

        using Authorization = std::variant<Authorized, Unauthorized, Forbidden>; ///< Owned admission decision for one HTTP request.

        /// Legacy notification-channel policy; modern requests never use legacy sessions.
        enum class LegacySubscriptions { Disabled, Enabled };

        /// Owned HTTP binding policy, copied into the router at construction.
        struct RouterOptions {
            LegacySubscriptions legacySubscriptions = LegacySubscriptions::Enabled; ///< Enable bounded 2025 session/GET channels.
        };

        /// Express router; copy/mount it while the borrowed registry and config remain alive.
        class Router : public express::Router {
        public:
            /// Runs on the event loop after Origin validation; exceptions become 500 without credential logging.
            using Authorize = std::function<Authorization(const std::shared_ptr<express::Request>&)>;

            /// Attach on the event loop after init; seals registration; empty authorize permits anonymous access.
            /// Retain server and config until this router and its operations have drained.
            Router(Server& server, Config& config, Authorize authorize = {}, RouterOptions options = {});
        };

    } // namespace http

} // namespace mcp::server

#endif
