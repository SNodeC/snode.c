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

#include "core/timer/Timer.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <string>
#include <vector>
#endif

namespace mcp::detail {
    TaskInfo taskInfo(const Json& value, ProtocolEra era);
    struct Task;
    class Tasks;

    /// The execution owner supplies transport and completion; Tasks owns task lifecycle.
    struct TaskExecution {
        using Output = std::function<void(const Json&, bool)>;
        virtual ~TaskExecution();
        std::optional<std::string> taskId() const;
        bool taskStatus(TaskState state, const std::string& message);
        virtual Completion result(Json value) = 0;
        virtual Completion error(int code, const std::string& message, Json data = nullptr) = 0;
        virtual void cancel(CancelReason reason = CancelReason::Client) = 0;
        virtual bool acceptInput(const std::string& key, const Json& value);
        virtual std::shared_ptr<const void> session() const = 0;
        virtual void sendTaskMessage(const Json& message) = 0;

        std::optional<Principal> principal;
        core::timer::Timer timer;
        Json clientCapabilities;
        Json serverInfo;
        std::optional<std::string> requestState;
        std::function<void()> resume;
        std::weak_ptr<Task> task;
        Json id;
        ProtocolEra era = ProtocolEra::Initialization20251125;
        bool done = false;
        Output output;
        utils::Timeval deadline;
    };

    struct Task {
        Json state;
        Json payload;
        Json inputs;
        std::set<std::string> usedInputs;
        std::shared_ptr<TaskExecution> execution;
        std::weak_ptr<Tasks> store;
        std::vector<std::shared_ptr<TaskExecution>> waiters;
        utils::Timeval expires;
        bool terminal() const;
        bool visible(const TaskExecution& request) const;
        Json info(bool detailed) const;
    };

    class Tasks : public std::enable_shared_from_this<Tasks> {
    public:
        ~Tasks();
        std::size_t perRequestor = 16;
        std::uint64_t maximumTtlMs = 86400000;
        std::function<void(const std::shared_ptr<Task>&)> notify;
        static bool negotiated(const Json& capabilities);
        Completion begin(const std::shared_ptr<TaskExecution>& execution, TaskOptions options);
        bool dispatch(const std::string& method, const Json& params, const std::shared_ptr<TaskExecution>& reply, std::size_t pageSize);
        void input(const std::shared_ptr<Task>& task, const Json& value);
        void working(const std::shared_ptr<Task>& task);
        void shutdown();
        void release(const std::shared_ptr<const void>& session);
        bool status(const std::shared_ptr<Task>& task, TaskState state, const std::string& message);

    private:
        void output(const std::shared_ptr<Task>& task, const Json& value, bool terminal);
        void publish(const std::shared_ptr<Task>& task);
        void expire(const std::string& id, CancelReason reason = CancelReason::Deadline);
        void result(const std::shared_ptr<Task>& task, const std::shared_ptr<TaskExecution>& reply);
        std::map<std::string, std::shared_ptr<Task>> entries;
        bool closing = false;
    };
} // namespace mcp::detail
