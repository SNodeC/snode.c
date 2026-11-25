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

#ifndef MCP_SERVER_CONFIG_H
#define MCP_SERVER_CONFIG_H

#include "utils/SubCommand.h"
#include "utils/Timeval.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    /// Noncopyable, nonmovable CLI owner; create before init and retain through transport drain.
    class Config : public utils::SubCommand {
    public:
        static constexpr std::string_view NAME = "mcp";                                 ///< Stable CLI subcommand name.
        static constexpr std::string_view DESCRIPTION = "MCP limits and Origin policy"; ///< CLI help text.

        /// Borrow parent through destruction; configure on the event-loop thread.
        explicit Config(utils::SubCommand* parent);

        ~Config() override; ///< Destroy after all attached transports have drained.

        Config(const Config&) = delete; ///< Stable CLI owner; copying is unsupported.

        /// Set the default before init; range 1..16777216; throws on invalid input; returns this.
        Config* setMaximumMessageBytes(std::size_t bytes);
        std::size_t getMaximumMessageBytes() const; ///< Read the effective message limit on the event loop.
        /// Set the default before init; range 1024..16777216; throws on invalid input; returns this.
        Config* setMaximumOutputBytes(std::size_t bytes);
        std::size_t getMaximumOutputBytes() const; ///< Read the effective output limit on the event loop.
        /// List page size, range 1..128; configure before init; CLI --page-size overrides it.
        Config* setPageSize(std::size_t size);
        std::size_t getPageSize() const;
        /// Set the default before init; range 1..300000 ms; throws on invalid input; returns this.
        Config* setDeadline(const utils::Timeval& deadline);
        utils::Timeval getDeadline() const; ///< Copy the effective timeout; new admissions capture it.
        /// Set owned default Origins before init; CLI values override defaults; returns this.
        Config* setAllowedOrigins(std::vector<std::string> origins);
        /// Borrow the authoritative list for this event-loop turn, never across reconfiguration.
        const std::vector<std::string>& getAllowedOrigins() const;

    private:
        struct Impl;

        std::unique_ptr<Impl> impl;
    };

} // namespace mcp::server

#endif
