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

#pragma once

#include "mcp/server/Config.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Types.h"

namespace CLI {
    class Option;
}

namespace utils {
    class SubCommand;
}

namespace mcp::server {
    struct ToolCall;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::mcp {

    class Headers;
    enum class LookupMode { Find, Read };
    std::shared_ptr<Headers> openHeaders(const std::string& root);
    void lookup(const std::shared_ptr<Headers>& headers,
                const ::mcp::server::ToolCall& call,
                const std::shared_ptr<::mcp::server::ToolReply>& reply,
                LookupMode mode,
                std::size_t files,
                std::size_t bytes);
    ::mcp::server::Json parseSchema(const std::string& name, const char* text, const char* field = "inputSchema");

    class DevelopmentConfig : public ::mcp::server::Config {
    public:
        explicit DevelopmentConfig(utils::SubCommand* parent);

        std::string getHeaderRoot() const;
        std::size_t getSearchFiles() const;
        std::size_t getSearchBytes() const;

    private:
        CLI::Option* headerRoot;
        CLI::Option* searchFiles;
        CLI::Option* searchBytes;
    };

    std::string instructions();

} // namespace apps::mcp
