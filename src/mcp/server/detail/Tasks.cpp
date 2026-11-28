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

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#include "mcp/detail/Tasks.h"

#include "jsonrpc/messages/Notification.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/server/Server.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace mcp::server {
    Server::Impl::Impl()
        : tasks(std::make_shared<detail::Tasks>()) {
        tasks->notify = [this](const auto& task) {
            taskChanged(task);
        };
    }

    void Server::Impl::endChannel(const std::shared_ptr<detail::Channel>& channel) const {
        channel->active = false;
        channel->onRootsChanged = {};
        channel->resources.clear();
        channel->output = {};
        tasks->release(channel);
    }

    void Server::Impl::taskChanged(const std::shared_ptr<detail::Task>& task) {
        if (task->execution->era == mcp::ProtocolEra::Initialization20251125) {
            if (task->execution->session()) {
                task->execution->sendTaskMessage(
                    jsonrpc::messages::Notification{"notifications/tasks/status", std::optional<Json>(task->info(false))}.toJson());
            }
        } else {
            std::vector<std::shared_ptr<detail::Response>> targets;
            for (const auto& weak : subscriptions) {
                if (const auto stream = weak.lock(); stream && !stream->done && task->visible(*stream)) {
                    const Json ids = stream->notifications.value("taskIds", Json::array());
                    if (std::find(ids.begin(), ids.end(), task->state["taskId"]) != ids.end()) {
                        targets.push_back(stream);
                    }
                }
            }
            for (const auto& stream : targets) {
                stream->notify("notifications/tasks", task->info(true));
            }
        }
    }
} // namespace mcp::server
