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

#ifndef MCP_SERVER_SERVER_H
#define MCP_SERVER_SERVER_H

#include "mcp/server/Prompt.h" // IWYU pragma: export
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Resource.h"         // IWYU pragma: export
#include "mcp/server/ResourceTemplate.h" // IWYU pragma: export
#include "mcp/server/Tool.h"             // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    namespace detail {
        class Access; ///< Module-private registry access.
    }

    /// Registration category used in immediate validation diagnostics.
    enum class RegistrationKind { Tool, Resource, Prompt, ResourceTemplate };

    /// Immediate registration failure; what() owns kind, identifier, field and reason.
    class RegistrationError : public std::invalid_argument {
    public:
        /// Construct an owned diagnostic; no borrowed strings survive the call.
        RegistrationError(RegistrationKind kind, std::string identifier, std::string field, std::string reason);

        ~RegistrationError() override; ///< Release the owned diagnostic; key function is supplied by the library.
    };

    /// Noncopyable, nonmovable registry owner; retain through attached transports' shutdown.
    class Server {
    public:
        /// Event-loop callback; retain the shared reply for asynchronous work.
        using ToolHandler = std::function<void(const ToolCall&, const std::shared_ptr<ToolReply>&)>;
        /// Resource callback on the event loop; shared completion owns request state.
        using ResourceHandler = std::function<void(const ResourceRead&, const std::shared_ptr<ResourceReply>&)>;
        /// Prompt callback on the event loop; shared completion owns request state.
        using PromptHandler = std::function<void(const PromptGet&, const std::shared_ptr<PromptReply>&)>;

        /// Construct before attachment; owns its name and registration data.
        explicit Server(std::string name, std::string version, std::string instructions = {});

        /// Destroy only after attached transports have drained.
        ~Server();

        Server(const Server&) = delete; ///< Stable owner identity; share through a reference instead.

        /// Register transactionally before attachment on the event loop; throws RegistrationError; returns this.
        Server* tool(ToolDefinition definition, ToolHandler handler);
        /// Retain shared ownership and validate transactionally before attachment; returns this.
        Server* tool(std::shared_ptr<Tool> handler);
        /// Register transactionally before attachment on the event loop; throws RegistrationError; returns this.
        Server* resource(ResourceDefinition definition, ResourceHandler handler);
        /// Retain shared ownership and validate transactionally before attachment; returns this.
        Server* resource(std::shared_ptr<Resource> handler);
        /// Validate and retain a template subclass before attachment; named errors are transactional.
        Server* resourceTemplate(std::shared_ptr<ResourceTemplate> handler);
        /// Convenience adapter over the same template registry; callbacks run on the event loop.
        Server* resourceTemplate(ResourceTemplateDefinition definition, ResourceHandler handler);
        /// Register transactionally before attachment on the event loop; throws RegistrationError; returns this.
        Server* prompt(PromptDefinition definition, PromptHandler handler);
        /// Retain shared ownership and validate transactionally before attachment; returns this.
        Server* prompt(std::shared_ptr<Prompt> handler);

    private:
        struct Impl;

        std::unique_ptr<Impl> impl;

        friend class detail::Access;
    };

} // namespace mcp::server

#endif
