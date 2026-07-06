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

#include "log/LogScopeOwner.h"

#include <utility>

namespace logger {

    LogScopeOwner::LogScopeOwner(LogOrigin origin,
                                 LogBoundary boundary,
                                 std::string component,
                                 std::optional<std::string> instance,
                                 std::optional<LogRole> role,
                                 std::optional<std::string> connection)
        : ownedScope{origin, boundary, std::move(component), std::move(instance), std::move(role), std::move(connection)} {
        if (ownedScope.role && !knownLogRole(*ownedScope.role)) {
            ownedScope.role = std::nullopt;
        }
        if (ownedScope.instance && ownedScope.instance->empty()) {
            ownedScope.instance = std::nullopt;
        }
        if (ownedScope.connection && ownedScope.connection->empty()) {
            ownedScope.connection = std::nullopt;
        }
    }

    LogScopeOwner LogScopeOwner::fromScope(LogScope scope) {
        return LogScopeOwner(copyLogScope(scope));
    }

    LogScope LogScopeOwner::scope() const noexcept {
        return viewLogScope(ownedScope);
    }

    BoundaryLogger LogScopeOwner::logger(BoundaryLogger::Sink sink, BoundaryLogger::Clock clock) const {
        return logger(std::move(sink), LogManager::effectiveLevel(scope()), std::move(clock));
    }

    BoundaryLogger LogScopeOwner::logger(BoundaryLogger::Sink sink, LogLevel threshold, BoundaryLogger::Clock clock) const {
        return BoundaryLogger(ownedScope, std::move(sink), threshold, std::move(clock));
    }

    LogScopeOwner::LogScopeOwner(OwnedLogScope scope)
        : ownedScope(std::move(scope)) {
    }

} // namespace logger
