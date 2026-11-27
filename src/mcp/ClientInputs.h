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
#include "mcp/Types.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>
#endif
namespace mcp {
    /// Owned, validated sampling parameters; model selection and permission remain client decisions.
    class SamplingRequest final {
    public:
        explicit SamplingRequest(Json params);
        const Json& toJson() const;

    private:
        Json value;
    };

    /// Owned sampled message, including tool-use blocks and provider metadata.
    class SamplingResult final {
    public:
        explicit SamplingResult(Json result);
        const Json& toJson() const;

    private:
        Json value;
    };

    /// Request the client's informational filesystem roots; this grants no file access.
    struct RootsRequest {};

    /// Owned validated file-URI roots, including optional names and metadata.
    class RootsResult final {
    public:
        explicit RootsResult(Json result);
        const Json& toJson() const;

    private:
        Json value;
    };

    using ClientInput = std::variant<FormInput, UrlInput, SamplingRequest, RootsRequest>;

    /// A mixed input round; uses nested RPCs in 2025 and input_required in 2026.
    /// Sampling and roots are deprecated, but still specified, in 2026-07-28.
    struct ClientInputRequired {
        std::map<std::string, ClientInput> requests;
        std::optional<std::string> requestState{};
    };
} // namespace mcp
