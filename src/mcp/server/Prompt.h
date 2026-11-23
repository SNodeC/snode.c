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

#ifndef MCP_SERVER_PROMPT_H
#define MCP_SERVER_PROMPT_H

#include "mcp/server/Reply.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <optional>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    struct PromptGet; ///< Borrowed request declared in Request.h.

    /// Whether a prompt argument must be present before its handler runs.
    enum class Requirement { Optional, Required };

    /// Owned prompt argument metadata; names must be nonempty and unique.
    struct PromptArgument {
        std::string name;                                ///< Owned argument name.
        std::string description{};                       ///< Owned human-readable description.
        Requirement requirement = Requirement::Optional; ///< Required arguments are checked before dispatch.
        CompletionHandler complete{};                    ///< Optional event-loop completer; never serialized into metadata.
        std::optional<std::string> title{};              ///< Optional owned display title.
    };

    /// Owned prompt metadata; its handler receives only string-valued arguments.
    struct PromptDefinition {
        std::string name;                        ///< Nonempty owned protocol identifier.
        std::string description{};               ///< Owned human-readable description.
        std::vector<PromptArgument> arguments{}; ///< Owned ordered argument declarations.
        std::optional<std::string> title{};      ///< Optional owned display title.
        std::vector<Icon> icons{};               ///< Owned optional icons; never fetched by the module.
        Json meta{};                             ///< Owned optional metadata object; null omits the field.
    };

    /// Application subclass; the server retains shared ownership until its destruction.
    class Prompt {
    public:
        /// Own the definition; construct before registration on the event-loop thread.
        explicit Prompt(PromptDefinition definition);

        /// Destroy on the event loop after requests using this object have drained.
        virtual ~Prompt();

        /// Borrow the immutable definition for this object's lifetime; event-loop access only.
        const PromptDefinition& getDefinition() const;
        /// Run on the event loop with validated input; retain the shared reply for async work.
        /// Complete at most once; request references are borrowed only through this call.
        virtual void get(const PromptGet& request, const std::shared_ptr<PromptReply>& reply) = 0;

    private:
        const PromptDefinition definition;
    };

} // namespace mcp::server

#endif
