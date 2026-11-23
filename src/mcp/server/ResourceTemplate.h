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

#ifndef MCP_SERVER_RESOURCETEMPLATE_H
#define MCP_SERVER_RESOURCETEMPLATE_H

#include "mcp/server/Resource.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    /// Owned variable declaration; must name a variable present in the URI template.
    struct TemplateVariable {
        std::string name;             ///< Owned variable name, unique within this template.
        CompletionHandler complete{}; ///< Optional event-loop completer; never serialized.
    };

    /// Owned scalar RFC 6570 template metadata; registration validates its expressions.
    struct ResourceTemplateDefinition {
        std::string uriTemplate;                   ///< Owned URI template supporting {var} and {+var}.
        std::string name;                          ///< Nonempty owned protocol name.
        std::string description{};                 ///< Owned human-readable description.
        std::string mimeType{};                    ///< Owned optional MIME type.
        std::optional<std::string> title{};        ///< Owned optional display title.
        std::vector<TemplateVariable> variables{}; ///< Owned completers, validated transactionally at registration.
    };

    /// Subclass retained by Server; construct and access only on the event-loop thread.
    class ResourceTemplate {
    public:
        /// Own immutable metadata; register before attaching any transport.
        explicit ResourceTemplate(ResourceTemplateDefinition definition);

        /// Release on the event loop after requests have drained; out-of-line key function.
        virtual ~ResourceTemplate();

        /// Borrow metadata for this object's lifetime; event-loop access only.
        const ResourceTemplateDefinition& getDefinition() const;
        /// Borrow the request for this call; retain reply for async work and complete once.
        virtual void read(const ResourceRead& request, const std::shared_ptr<ResourceReply>& reply) = 0;

    private:
        const ResourceTemplateDefinition definition;
    };

} // namespace mcp::server

#endif // MCP_SERVER_RESOURCETEMPLATE_H
