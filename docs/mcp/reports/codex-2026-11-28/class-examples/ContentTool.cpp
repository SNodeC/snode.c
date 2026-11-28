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

#include "core/timer/Timer.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "utils/Uuid.h"
#include "utils/base64.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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

class ContentTool final : public Tool {
public:
    ContentTool()
        : Tool({.name = "content",
                .description = "Every content variant: text, image, audio, resource link and embedded text/blob.",
                .inputSchema = Json::parse(R"json(
{
    "type": "object",
    "additionalProperties": false
}
)json"),
                .title = "content",
                .annotations =
                    Json::parse(R"json(
{
    "readOnlyHint": true,
    "destructiveHint": false,
    "idempotentHint": true,
    "openWorldHint": false
}
)json"),
                .icons = {icon()},
                .meta = Json::parse(R"json(
{
    "showcase/feature": "content"
}
)json")}) {
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

} // namespace apps::mcp::features
