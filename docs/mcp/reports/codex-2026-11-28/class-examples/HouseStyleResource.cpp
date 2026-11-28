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

#include "core/system/unistd.h"
#include "core/timer/Timer.h"
#include "mcp/Types.h"
#include "mcp/server/Config.h"
#include "mcp/server/Prompt.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Resource.h"
#include "mcp/server/ResourceTemplate.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "mcp/server/Tool.h"
#include "mcp/server/Types.h"
#include "utils/CLI11.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <dirent.h>
#include <exception>
#include <fcntl.h>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

#ifndef MCP_HEADER_ROOT
#define MCP_HEADER_ROOT "/usr/local/include/snode.c"
#endif

namespace apps::mcp {

    using ::mcp::server::Json;
    using ::mcp::server::ToolReply;
    using ::mcp::server::ToolResult;

constexpr const char* style = "SNode.C: use one event loop; configure named instances before init; listen, then start. "
                              "Use semantic logging, express routing and existing CLI11 instance options. "
                              "Keep protocol code independent of transports. Bound every handler's work. "
                              "Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream.";

std::string instructions() {
    return style;
}

class HouseStyleResource : public ::mcp::server::Resource {
public:
    HouseStyleResource()
        : Resource({"snodec://development/house-style", "SNode.C house style", {}, "text/plain"}) {
    }

    void read(const ::mcp::server::ResourceRead& request, const std::shared_ptr<::mcp::server::ResourceReply>& reply) override {
        reply->result(::mcp::server::ResourceResult{{::mcp::server::TextResource{request.uri, "text/plain", instructions()}}});
    }
};

} // namespace apps::mcp
