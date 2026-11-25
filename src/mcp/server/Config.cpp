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

#include "mcp/server/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cmath>
#include <functional>
#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    struct Config::Impl {
        CLI::Option* message;
        CLI::Option* output;
        CLI::Option* deadline;
        CLI::Option* pageSize;
        CLI::Option* origins;

        std::vector<std::string> allowedOrigins;
    };

    Config::Config(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Applications")
        , impl(std::make_unique<Impl>()) {
        impl->pageSize = addOption("--page-size", "Maximum entries per MCP list page", "entries", std::size_t{128}, CLI::Range(1, 128));
        impl->message =
            addOption("--maximum-message-bytes", "Maximum MCP message bytes", "bytes", std::size_t{1048576}, CLI::Range(1, 16777216));
        impl->output = addOption(
            "--maximum-output-bytes", "Maximum queued MCP output bytes", "bytes", std::size_t{1048576}, CLI::Range(1024, 16777216));
        const CLI::Range range(1.0, 300000.0);
        const CLI::Validator deadline(
            [range](std::string& value) -> std::string {
                const std::string error = range(value);
                return error.empty() && !std::isfinite(std::stod(value)) ? "Finite deadline required" : error;
            },
            range.get_description());
        impl->deadline = addOption("--deadline-ms", "Ordinary request deadline", "milliseconds", 30000.0, deadline);
        impl->origins =
            addOptionVariable(
                "--allowed-origin", impl->allowedOrigins, "Exact allowed browser Origin (repeatable)", "origin", CLI::Validator())
                ->expected(0, -1)
                ->force_callback();
        setDefaultValue(impl->origins, std::vector<std::string>{});
    }

    Config::~Config() = default;

    std::size_t Config::getMaximumMessageBytes() const {
        return impl->message->as<std::size_t>();
    }

    Config* Config::setMaximumMessageBytes(std::size_t bytes) {
        if (!setDefaultValue(impl->message, bytes)) {
            throw std::invalid_argument("Invalid maximum-message-bytes");
        }

        return this;
    }

    std::size_t Config::getMaximumOutputBytes() const {
        return impl->output->as<std::size_t>();
    }

    Config* Config::setMaximumOutputBytes(std::size_t bytes) {
        if (!setDefaultValue(impl->output, bytes)) {
            throw std::invalid_argument("Invalid maximum-output-bytes");
        }

        return this;
    }

    std::size_t Config::getPageSize() const {
        return impl->pageSize->as<std::size_t>();
    }

    Config* Config::setPageSize(std::size_t size) {
        if (!setDefaultValue(impl->pageSize, size)) {
            throw std::invalid_argument("Invalid page-size");
        }
        return this;
    }

    utils::Timeval Config::getDeadline() const {
        return utils::Timeval(impl->deadline->as<double>() / 1000.0);
    }

    Config* Config::setDeadline(const utils::Timeval& deadline) {
        if (!setDefaultValue(impl->deadline, deadline.getMsd())) {
            throw std::invalid_argument("Invalid deadline-ms");
        }

        return this;
    }

    const std::vector<std::string>& Config::getAllowedOrigins() const {
        return impl->allowedOrigins;
    }

    Config* Config::setAllowedOrigins(std::vector<std::string> origins) {
        setDefaultValue(impl->origins, origins);

        return this;
    }

} // namespace mcp::server
