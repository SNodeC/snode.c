// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "FindTool.h"

#include "Tools.h"

#include <nlohmann/json.hpp>
#include <string>
namespace apps::mcp {
    FindTool::FindTool(const DevelopmentConfig& config)
        : Tool({"api.find",
                "Literal search of confined installed headers; returns relative paths, lines and byte offsets.",
                parseSchema("api.find", R"json(
{
    "type": "object",
    "properties": {
        "query": {
            "type": "string",
            "maxLength": 128,
            "minLength": 1
        },
        "limit": {
            "type": "integer",
            "default": 20,
            "maximum": 100,
            "minimum": 1
        }
    },
    "required": [
        "query"
    ],
    "additionalProperties": false
}
)json")})
        , headers(openHeaders(config.getHeaderRoot()))
        , files(config.getSearchFiles())
        , bytes(config.getSearchBytes()) {
    }
    void FindTool::call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) {
        lookup(headers, request, reply, LookupMode::Find, files, bytes);
    }
} // namespace apps::mcp
