// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_RESULTS_H
#define MCP_SERVER_RESULTS_H
#include "mcp/server/Types.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>
namespace mcp::server {
    /// Owned UTF-8 resource contents; the URI identifies the resource.
    struct TextResource {
        std::string uri;      ///< Owned resource URI.
        std::string mimeType; ///< Owned MIME media type.
        std::string text;     ///< Owned UTF-8 text.
    };
    /// Owned binary resource contents encoded as base64.
    struct BlobResource {
        std::string uri;      ///< Owned resource URI.
        std::string mimeType; ///< Owned MIME media type.
        std::string base64;   ///< Owned base64 data, encoded by the application.
    };
    using ResourceContents = std::variant<TextResource, BlobResource>; ///< Owned text or binary resource value.
    /// Owned UTF-8 tool or prompt content.
    struct TextContent {
        std::string text;        ///< Owned UTF-8 text.
        Annotations annotations; ///< Owned optional annotations for this block.
    };
    /// Owned base64 image with an explicit media type.
    struct ImageContent {
        std::string base64;      ///< Owned base64 data, encoded by the application.
        std::string mimeType;    ///< Owned MIME media type.
        Annotations annotations; ///< Owned optional annotations for this block.
    };
    /// Owned base64 audio with an explicit media type.
    struct AudioContent {
        std::string base64;      ///< Owned base64 data, encoded by the application.
        std::string mimeType;    ///< Owned MIME media type.
        Annotations annotations; ///< Owned optional annotations for this block.
    };
    /// Owned resource reference; reading it requires a separate resource request.
    struct ResourceLink {
        std::string uri;                   ///< Owned resource URI.
        std::string name;                  ///< Owned human-readable resource name.
        std::string description;           ///< Owned optional human-readable description.
        std::string mimeType;              ///< Owned MIME media type.
        std::optional<std::uint64_t> size; ///< Optional resource size in bytes.
        Annotations annotations;           ///< Owned optional annotations for this block.
    };
    /// Owned resource data embedded directly in tool or prompt content.
    struct EmbeddedResource {
        ResourceContents resource; ///< Owned embedded text or binary resource.
        Annotations annotations;   ///< Owned optional annotations for this block.
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

    private:
        explicit ToolResult(Json value);
        template <class Result>
        friend class Reply;
        Json value;
    };
    /// Owned resources returned by one successful resource read.
    struct ResourceResult {
        std::vector<ResourceContents> contents; ///< Owned resource contents in response order.
    };
    /// Owned ordered messages returned by one successful prompt expansion.
    struct PromptResult {
        std::optional<std::string> description; ///< Owned optional human-readable description.
        std::vector<PromptMessage> messages;    ///< Owned prompt messages in presentation order.
    };
} // namespace mcp::server
#endif
