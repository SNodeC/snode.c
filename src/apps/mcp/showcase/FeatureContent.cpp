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

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#include "Features.h"
#include "mcp/Types.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "utils/base64.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::mcp::features {

    using namespace ::mcp::server;

    std::string encoded(const std::string& bytes) {
        return base64::base64_encode(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
    }

    Icon icon() {
        const std::string svg =
            R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><rect width="16" height="16" fill="#247"/><circle cx="8" cy="8" r="5" fill="#fff"/></svg>)svg";
        return Icon{"data:image/svg+xml;base64," + encoded(svg), "image/svg+xml", {"16x16"}, IconTheme::Light};
    }

    Implementation implementation(std::string version) {
        return Implementation{"snodec-mcp-showcase",
                              std::move(version),
                              "SNode.C MCP feature showcase",
                              "An executable tour of implemented MCP server and client features.",
                              "https://github.com/SNodeC/snode.c",
                              {icon()}};
    }

    Json schema(const std::string& text) {
        return nlohmann::ordered_json::parse(text);
    }

    ToolDefinition toolDefinition(std::string name, std::string description, Json input) {
        ToolDefinition result{std::move(name), std::move(description), std::move(input)};
        result.title = result.name;
        result.icons = {icon()};
        result.annotations = {{"readOnlyHint", true}, {"destructiveHint", false}, {"idempotentHint", true}, {"openWorldHint", false}};
        result.meta = {{"showcase/feature", result.name}};
        return result;
    }

    Json emptySchema() {
        return schema(R"json(
{
    "type": "object",
    "additionalProperties": false
}
)json");
    }

    namespace {

        class TextTool final : public Tool {
        public:
            TextTool()
                : Tool(toolDefinition("text", "Plain text results and tool annotations.", emptySchema())) {
            }

            void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
                reply->result(ToolResult::text("Hello from the MCP showcase.").setMeta({{"showcase/result", "text"}}));
            }
        };

        class ToolErrorTool final : public Tool {
        public:
            ToolErrorTool()
                : Tool(toolDefinition("tool.error", "A tool failure: isError, not a JSON-RPC envelope error.", emptySchema())) {
            }

            void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
                reply->result(ToolResult::toolError("Deliberate tool failure."));
            }
        };

        class ProtocolErrorTool final : public Tool {
        public:
            ProtocolErrorTool()
                : Tool(toolDefinition("protocol.error", "An explicit JSON-RPC InvalidParams error.", emptySchema())) {
            }

            void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
                reply->error(ErrorCode::InvalidParams, "Deliberate protocol error.", {{"feature", "errors"}});
            }
        };

        ToolDefinition structuredDefinition(bool valid) {
            auto tool = toolDefinition(
                valid ? "structured.valid" : "structured.invalid", "Output-schema enforcement and structuredContent.", emptySchema());
            tool.outputSchema = schema(R"json(
{
    "type": "object",
    "properties": {
        "answer": {
            "type": "integer",
            "const": 42
        }
    },
    "required": [
        "answer"
    ],
    "additionalProperties": false
}
)json");
            return tool;
        }

        class StructuredTool final : public Tool {
        public:
            StructuredTool(bool valid)
                : Tool(structuredDefinition(valid))
                , valid(valid) {
            }

            void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
                reply->result(ToolResult::json({{"answer", valid ? 42 : 0}}));
            }

        private:
            bool valid;
        };

        class SchemaTool final : public Tool {
        public:
            SchemaTool()
                : Tool(toolDefinition(
                      "schema", "JSON Schema 2020-12: refs, oneOf, const/enum, Unicode and unevaluatedProperties.", schema(R"json(
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "$defs": {
        "word": {
            "type": "string",
            "pattern": "^\\p{L}+$"
        }
    },
    "properties": {
        "word": {
            "$ref": "#/$defs/word"
        },
        "choice": {
            "oneOf": [
                {
                    "const": "alpha"
                },
                {
                    "type": "integer",
                    "enum": [
                        1,
                        2
                    ]
                }
            ]
        }
    },
    "required": [
        "word",
        "choice"
    ],
    "unevaluatedProperties": false
}
)json"))) {
            }

            void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
                reply->result(ToolResult::json(call.arguments));
            }
        };

        class HeadersTool final : public Tool {
        public:
            HeadersTool()
                : Tool(toolDefinition("headers", "2026 HTTP x-mcp-header mirroring (ordinary argument in 2025 and stdio).", schema(R"json(
{
    "type": "object",
    "properties": {
        "region": {
            "type": "string",
            "x-mcp-header": "Region"
        }
    },
    "required": [
        "region"
    ],
    "additionalProperties": false
}
)json"))) {
            }

            void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
                reply->result(ToolResult::json(call.arguments));
            }
        };

        class ContentTool final : public Tool {
        public:
            ContentTool()
                : Tool(toolDefinition(
                      "content", "Every content variant: text, image, audio, resource link and embedded text/blob.", emptySchema())) {
            }

            void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
                const Annotations annotation{{Role::User, Role::Assistant}, 0.5, "2026-01-01T00:00:00Z"};
                const std::string svg =
                    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1\" height=\"1\"><rect width=\"1\" height=\"1\"/></svg>";
                std::string wav = "RIFF";
                const auto little = [&wav](std::uint32_t value, unsigned width) {
                    for (unsigned i = 0; i < width; ++i) {
                        wav.push_back(static_cast<char>((value >> (8U * i)) & 255U));
                    }
                };
                little(38, 4);
                wav += "WAVEfmt ";
                little(16, 4);
                little(1, 2);
                little(1, 2);
                little(8000, 4);
                little(16000, 4);
                little(2, 2);
                little(16, 2);
                wav += "data";
                little(2, 4);
                little(0, 2);
                reply->result(ToolResult::content(
                    {TextContent{"Original generated media; one silent audio sample.", annotation},
                     ImageContent{encoded(svg), "image/svg+xml", annotation},
                     AudioContent{encoded(wav), "audio/wav", annotation},
                     ResourceLink{
                         "showcase://text", "Showcase text", "A readable resource", "text/plain", 8, annotation, {}, "Text", {icon()}},
                     EmbeddedResource{TextResource{"showcase://embedded/text", "text/plain", "embedded"}, annotation},
                     EmbeddedResource{BlobResource{"showcase://embedded/blob", "application/octet-stream", encoded("SNode.C")},
                                      annotation}}));
            }
        };

        class PrincipalTool final : public Tool {
        public:
            PrincipalTool()
                : Tool(toolDefinition(
                      "principal", "Application-authorized identity from RequestContext, never from arguments.", emptySchema())) {
            }

            void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
                const auto& principal = call.context.getPrincipal();
                reply->result(ToolResult::json({{"subject", principal ? principal->subject : "anonymous"}}));
            }
        };

    } // namespace

    void registerContent(Server& server) {
        server.tool(std::make_shared<TextTool>());
        server.tool(std::make_shared<ToolErrorTool>());
        server.tool(std::make_shared<ProtocolErrorTool>());
        server.tool(std::make_shared<StructuredTool>(true));
        server.tool(std::make_shared<StructuredTool>(false));
        server.tool(std::make_shared<SchemaTool>());
        server.tool(std::make_shared<HeadersTool>());
        server.tool(std::make_shared<ContentTool>());
        server.tool(std::make_shared<PrincipalTool>());
    }

} // namespace apps::mcp::features
