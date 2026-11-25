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

#include "web/http/client/tools/EventSource.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <algorithm>
#include <cstddef>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client::tools {

    bool EventSource::digits(std::string_view value) {
        bool valid = !value.empty();
        for (const char digit : value) {
            valid = valid && digit >= '0' && digit <= '9';
        }
        return valid;
    }

    uint32_t EventSource::parseU32(std::string_view value) {
        uint64_t result = 0;
        for (const char digit : value) {
            result = std::min<uint64_t>(0xFFFFFFFFULL, result * 10 + static_cast<unsigned char>(digit) - '0');
        }
        return static_cast<uint32_t>(result);
    }

    void EventSource::deliverMessage(const std::shared_ptr<SharedState>& sharedState,
                                     const std::string& evType,
                                     const std::string& payload,
                                     const std::string& evId) {
        const MessageEvent e{evType, payload, evId, sharedState->origin};

        if (auto it = sharedState->onEventListener.find(evType); it != sharedState->onEventListener.end()) {
            for (const auto& onEventListener : it->second) {
                onEventListener(e);
            }
        }

        if (evType == "message") {
            for (const auto& onMessageListener : sharedState->onMessageListener) {
                onMessageListener(e);
            }
        }
    }

    void EventSource::dispatch(const std::shared_ptr<SharedState>& state) {
        state->lastId = state->idBuf;
        if (!state->data.empty()) {
            state->data.pop_back();
            const std::string type = state->type.empty() ? "message" : state->type;
            const std::string payload = std::exchange(state->data, std::string{});
            deliverMessage(state, type, payload, state->lastId);
        }
        state->type.clear();
    }

    bool EventSource::processLine(const std::shared_ptr<SharedState>& sharedState, std::string_view line) {
        bool success = true;

        if (!line.empty() && line.front() != ':') {
            std::string_view field;
            std::string_view value;

            if (auto p = line.find(':'); p != std::string_view::npos) {
                field = line.substr(0, p);
                value = line.substr(p + 1);
                if (!value.empty() && value.front() == ' ') {
                    value.remove_prefix(1);
                }
            } else {
                field = line;
            }

            if (field == "data") {
                static constexpr std::size_t kMaxDataSize = 1 << 20;

                success = sharedState->data.size() + value.size() + 1 <= kMaxDataSize; // Data length

                if (success) {
                    sharedState->data.append(value);
                    sharedState->data.push_back('\n');
                }
            } else if (field == "event") {
                sharedState->type = value;
            } else if (field == "id") {
                if (value.find('\0') == std::string_view::npos) {
                    sharedState->idBuf = value;
                }
            } else if (field == "retry") {
                if (digits(value)) {
                    uint32_t const retry = parseU32(value);

                    sharedState->retry = retry;
                }
            }
        }

        return success;
    }

    bool EventSource::parse(const std::shared_ptr<SharedState>& sharedState) {
        static constexpr std::size_t kMaxLine = 256 * 1024;

        bool success = true;
        bool eol = true;
        do {
            const std::size_t i = sharedState->pending.find_first_of("\r\n");

            eol = i != std::string::npos && (sharedState->pending[i] != '\r' || i + 1 < sharedState->pending.size());

            if (eol && i <= kMaxLine) {
                const std::size_t eolLength =
                    (sharedState->pending[i] == '\r' && i + 1 < sharedState->pending.size() && sharedState->pending[i + 1] == '\n') ? 2 : 1;

                const std::string_view line(sharedState->pending.data(), i);

                if (!line.empty()) {
                    success = processLine(sharedState, line);
                } else {
                    dispatch(sharedState);
                }

                sharedState->pending.erase(0, i + eolLength);
            } else {
                success = sharedState->pending.size() <= kMaxLine; // Line length
            }
        } while (eol && success);

        return success;
    }

    bool EventSource::feed(const std::shared_ptr<SharedState>& state, std::string_view chunk) {
        state->pending.append(chunk);
        return parse(state);
    }

    EventSource::EventSource()
        : sharedState(std::make_shared<SharedState>()) {
    }

    EventSource::~EventSource() {
    }

    EventSource* EventSource::onMessage(std::function<void(const MessageEvent&)> messageCallback) {
        sharedState->onMessageListener.push_back(std::move(messageCallback));

        return this;
    }

    EventSource* EventSource::addEventListener(const std::string& key, std::function<void(const MessageEvent&)> eventListener) {
        sharedState->onEventListener[key].push_back(std::move(eventListener));

        return this;
    }

    EventSource* EventSource::removeEventListeners(const std::string& type) {
        sharedState->onEventListener.erase(type);

        return this;
    }

    EventSource* EventSource::onOpen(std::function<void()> onOpen) {
        sharedState->onOpenListener.push_back(std::move(onOpen));

        return this;
    }

    EventSource* EventSource::onError(std::function<void()> onError) {
        sharedState->onErrorListener.push_back(std::move(onError));

        return this;
    }

    EventSource::ReadyState EventSource::readyState() const {
        return sharedState->ready;
    }

    const std::string& EventSource::lastEventId() const {
        return sharedState->lastId;
    }

    uint32_t EventSource::retry() const {
        return sharedState->retry;
    }

    EventSource* EventSource::retry(uint32_t retry) {
        sharedState->retry = retry;

        return this;
    }
} // namespace web::http::client::tools
