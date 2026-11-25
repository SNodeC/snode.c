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

#ifndef MCP_RESULTS_H
#define MCP_RESULTS_H

#include "mcp/Types.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp {

    /// Owned completion suggestions; Reply enforces the wire limit of 100 values.
    struct CompletionResult {
        std::vector<std::string> values;    ///< Ranked suggestions, owned until completion.
        std::optional<std::size_t> total{}; ///< Optional total match count, at least the returned count.
        bool hasMore = false;               ///< True when additional matching values exist.
        Json meta{};                        ///< Optional owned metadata object; null omits it.
    };

    /// Owned UTF-8 resource contents; the URI identifies the resource.
    struct TextResource {
        std::string uri;      ///< Owned resource URI.
        std::string mimeType; ///< Owned MIME media type.
        std::string text;     ///< Owned UTF-8 text.
        Json meta{};          ///< Optional owned metadata object; null omits it.
    };

    /// Owned binary resource contents encoded as base64.
    struct BlobResource {
        std::string uri;      ///< Owned resource URI.
        std::string mimeType; ///< Owned MIME media type.
        std::string base64;   ///< Owned base64 data, encoded by the application.
        Json meta{};          ///< Optional owned metadata object; null omits it.
    };

    using ResourceContents = std::variant<TextResource, BlobResource>; ///< Owned text or binary resource value.

    /// Owned UTF-8 tool or prompt content.
    struct TextContent {
        std::string text;        ///< Owned UTF-8 text.
        Annotations annotations; ///< Owned optional annotations for this block.
        Json meta{};             ///< Optional owned metadata object; null omits it.
    };

    /// Owned base64 image with an explicit media type.
    struct ImageContent {
        std::string base64;      ///< Owned base64 data, encoded by the application.
        std::string mimeType;    ///< Owned MIME media type.
        Annotations annotations; ///< Owned optional annotations for this block.
        Json meta{};             ///< Optional owned metadata object; null omits it.
    };

    /// Owned base64 audio with an explicit media type.
    struct AudioContent {
        std::string base64;      ///< Owned base64 data, encoded by the application.
        std::string mimeType;    ///< Owned MIME media type.
        Annotations annotations; ///< Owned optional annotations for this block.
        Json meta{};             ///< Optional owned metadata object; null omits it.
    };

    /// Owned resource reference; reading it requires a separate resource request.
    struct ResourceLink {
        std::string uri;                    ///< Owned resource URI.
        std::string name;                   ///< Owned human-readable resource name.
        std::string description;            ///< Owned optional human-readable description.
        std::string mimeType;               ///< Owned MIME media type.
        std::optional<std::uint64_t> size;  ///< Optional resource size in bytes.
        Annotations annotations;            ///< Owned optional annotations for this block.
        Json meta{};                        ///< Optional owned metadata object; null omits it.
        std::optional<std::string> title{}; ///< Optional display title.
        std::vector<Icon> icons{};          ///< Optional display icons.
    };

    /// Owned resource data embedded directly in tool or prompt content.
    struct EmbeddedResource {
        ResourceContents resource; ///< Owned embedded text or binary resource.
        Annotations annotations;   ///< Owned optional annotations for this block.
        Json meta{};               ///< Optional owned metadata object; null omits it.
    };

    using Content =
        std::variant<TextContent, ImageContent, AudioContent, ResourceLink, EmbeddedResource>; ///< Owned protocol content block, serialized
                                                                                               ///< by the module.

    /// Prompt messages accept content, not a prebuilt protocol envelope.
    struct PromptMessage {
        Role role;       ///< Sender of this prompt message.
        Content content; ///< Owned message content.
    };

    /// Owned tool completion; factories derive all wire fields from one value.
    class ToolResult {
    public:
        /// Own text as a value before init; submit through an event-loop reply.
        static ToolResult text(std::string text);
        /// Own one JSON value before init; event-loop completion encodes both channels consistently.
        static ToolResult json(Json value);
        /// Own mixed image/audio/resource content before init; submit through an event-loop reply.
        static ToolResult content(std::vector<Content> content);
        /// Own a tool-failure message before init; submit through an event-loop reply.
        static ToolResult toolError(std::string message);
        static ToolResult toolError(std::vector<Content> content); ///< Owned mixed failure content; event-loop completion reports isError.

        /// Own an optional metadata object; throws invalid_argument for non-object/non-null values.
        ToolResult& setMeta(Json meta);
        /// Serialize owned protocol result data without a JSON-RPC envelope.
        Json toJson() const;

    private:
        explicit ToolResult(Json value);

        Json value;
    };

    /// Owned resources returned by one successful resource read.
    struct ResourceResult {
        std::vector<ResourceContents> contents; ///< Owned resource contents in response order.
        Json meta{};                            ///< Optional owned metadata object; null omits it.
        std::optional<CacheHints> cache{};      ///< Optional modern cache policy; omitted in the 2025 era.
    };

    /// Owned ordered messages returned by one successful prompt expansion.
    struct PromptResult {
        std::optional<std::string> description; ///< Owned optional human-readable description.
        std::vector<PromptMessage> messages;    ///< Owned prompt messages in presentation order.
        Json meta{};                            ///< Optional owned metadata object; null omits it.
    };

} // namespace mcp

#endif
