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
#include "utils/Timeval.h"

#include <cstddef>
#include <nlohmann/json.hpp>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#endif
namespace mcp::server::detail {
    struct Response;
    class Tasks;

    struct Task {
        Json state;
        Json payload;
        Json inputs;
        std::set<std::string> usedInputs;
        std::shared_ptr<Response> execution;
        std::weak_ptr<Tasks> store;
        std::vector<std::weak_ptr<Response>> waiters;
        utils::Timeval expires;
        bool terminal() const;
        bool visible(const Response& request) const;
        Json info(bool detailed) const;
    };

    class Tasks : public std::enable_shared_from_this<Tasks> {
    public:
        ~Tasks();
        std::function<void(const std::shared_ptr<Task>&)> notify;
        static bool negotiated(const Json& capabilities);
        Completion begin(const std::shared_ptr<Response>& execution, TaskOptions options);
        bool dispatch(const std::string& method, const Json& params, const std::shared_ptr<Response>& reply, std::size_t pageSize);
        void input(const std::shared_ptr<Task>& task, const Json& value);
        void working(const std::shared_ptr<Task>& task);
        void shutdown();

    private:
        void output(const std::shared_ptr<Task>& task, const Json& value, bool terminal);
        void publish(const std::shared_ptr<Task>& task);
        void expire(const std::string& id);
        void result(const std::shared_ptr<Task>& task, const std::shared_ptr<Response>& reply);
        std::map<std::string, std::shared_ptr<Task>> entries;
        bool closing = false;
    };
} // namespace mcp::server::detail
