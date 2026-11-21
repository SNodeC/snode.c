// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "ReadTool.h"

#include "Tools.h"

#include <nlohmann/json.hpp>
#include <string>
namespace apps::mcp {
    ReadTool::ReadTool(const DevelopmentConfig& config)
        : Tool({"api.read",
                "Read a bounded UTF-8 header range using a relative path and optional byte offset.",
                parseSchema("api.read", R"json(
{
    "type": "object",
    "properties": {
        "path": {
            "type": "string",
            "maxLength": 512,
            "minLength": 1
        },
        "offset": {
            "type": "integer",
            "maximum": 67108864,
            "minimum": 0
        },
        "bytes": {
            "type": "integer",
            "maximum": 65536,
            "minimum": 1
        }
    },
    "required": [
        "path"
    ],
    "additionalProperties": false
}
)json")})
        , headers(openHeaders(config.getHeaderRoot())) {
    }
    void ReadTool::call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) {
        lookup(headers, request, reply, LookupMode::Read, 1, 65536);
    }
} // namespace apps::mcp
