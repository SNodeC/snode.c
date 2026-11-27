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

#include "mcp/ClientInputs.h"

#include "mcp/Schema.h"
#include "mcp/detail/ClientInputs.h"
#include "mcp/detail/Serialization.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cctype>
#include <cmath>
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <nlohmann/json.hpp>
#include <set>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#endif
namespace mcp {
    namespace {
        void require(bool valid, const char* message) {
            if (!valid) {
                throw std::invalid_argument(message);
            }
        }

        void samplingMessage(const Json& message) {
            require(message.is_object() && (message.value("role", Json()) == "user" || message.value("role", Json()) == "assistant"),
                    "Invalid sampling role");
            const auto& content = message.at("content");
            const Json blocks = content.is_array() ? content : Json::array({content});
            require(!blocks.empty(), "Empty sampling content");
            std::set<std::string> ids;
            for (const auto& block : blocks) {
                const std::string type = block.at("type").get<std::string>();
                if (type == "tool_use") {
                    require(message["role"] == "assistant" && block.at("input").is_object() && block.at("name").is_string() &&
                                block.at("id").is_string() && ids.insert(block["id"].get<std::string>()).second,
                            "Invalid sampling tool use");
                } else if (type == "tool_result") {
                    require(message["role"] == "user" && block.at("toolUseId").is_string() && block.at("content").is_array() &&
                                (!block.contains("isError") || block["isError"].is_boolean()) &&
                                (!block.contains("structuredContent") || block["structuredContent"].is_object()),
                            "Invalid sampling tool result");
                    for (const auto& item : block["content"]) {
                        static_cast<void>(detail::decodeContent(item));
                    }
                } else {
                    require(type == "text" || type == "image" || type == "audio", "Unsupported sampling content");
                    static_cast<void>(detail::decodeContent(block));
                }
            }
        }
    } // namespace

    SamplingRequest::SamplingRequest(Json params)
        : value(std::move(params)) {
        require(value.is_object() && value.at("messages").is_array() && value.at("maxTokens").is_number() &&
                    value["maxTokens"].get<double>() > 0 && std::isfinite(value["maxTokens"].get<double>()),
                "Invalid sampling parameters");
        value.erase("task"); // Unadvertised augmentation is ignored; sampling completes normally.
        std::set<std::string> expected;
        for (const auto& message : value["messages"]) {
            samplingMessage(message);
            const Json blocks = message["content"].is_array() ? message["content"] : Json::array({message["content"]});
            std::set<std::string> uses;
            std::set<std::string> results;
            for (const auto& block : blocks) {
                if (block["type"] == "tool_use") {
                    uses.insert(block["id"].get<std::string>());
                }
                if (block["type"] == "tool_result") {
                    require(results.insert(block["toolUseId"].get<std::string>()).second, "Duplicate sampling tool result");
                }
            }
            require(results == expected && (results.empty() || results.size() == blocks.size()),
                    "Unbalanced or mixed sampling tool results");
            expected = std::move(uses);
        }
        require(expected.empty(), "Sampling tool uses require following results");
        for (const auto* field : {"systemPrompt", "includeContext"}) {
            require(!value.contains(field) || value[field].is_string(), "Invalid sampling string option");
        }
        if (value.contains("includeContext")) {
            require(value["includeContext"] == "none" || value["includeContext"] == "thisServer" || value["includeContext"] == "allServers",
                    "Invalid sampling context");
        }
        require(!value.contains("temperature") || (value["temperature"].is_number() && std::isfinite(value["temperature"].get<double>())),
                "Invalid sampling temperature");
        for (const auto* field : {"metadata", "_meta", "modelPreferences"}) {
            require(!value.contains(field) || value[field].is_object(), "Invalid sampling metadata/preferences");
        }
        if (value.contains("stopSequences")) {
            static_cast<void>(value["stopSequences"].get<std::vector<std::string>>());
        }
        if (value.contains("modelPreferences")) {
            const auto& preferences = value["modelPreferences"];
            for (const auto* field : {"costPriority", "speedPriority", "intelligencePriority"}) {
                require(!preferences.contains(field) || (preferences[field].is_number() && preferences[field].get<double>() >= 0 &&
                                                         preferences[field].get<double>() <= 1),
                        "Invalid sampling model priority");
            }
            if (preferences.contains("hints")) {
                require(preferences["hints"].is_array(), "Invalid sampling model hints");
                for (const auto& hint : preferences["hints"]) {
                    require(hint.is_object() && (!hint.contains("name") || hint["name"].is_string()), "Invalid sampling model hint");
                }
            }
        }
        if (value.contains("tools")) {
            require(value["tools"].is_array(), "Invalid sampling tools");
            std::set<std::string> names;
            for (const auto& tool : value["tools"]) {
                std::string error;
                require(tool.at("name").is_string() && names.insert(tool["name"].get<std::string>()).second &&
                            tool.at("inputSchema").is_object() && tool["inputSchema"].value("type", Json()) == "object" &&
                            Schema::check(tool["inputSchema"], error) &&
                            (!tool.contains("outputSchema") || Schema::check(tool["outputSchema"], error)),
                        "Invalid sampling tool definition");
            }
        }
        if (value.contains("toolChoice")) {
            require(value["toolChoice"].is_object(), "Invalid sampling tool choice");
            const Json mode = value["toolChoice"].value("mode", Json("auto"));
            require(mode == "auto" || mode == "required" || mode == "none", "Invalid sampling tool choice mode");
        }
    }

    const Json& SamplingRequest::toJson() const {
        return value;
    }

    SamplingResult::SamplingResult(Json result)
        : value(std::move(result)) {
        samplingMessage(value);
        require(value.at("model").is_string() && (!value.contains("stopReason") || value["stopReason"].is_string()) &&
                    (!value.contains("_meta") || value["_meta"].is_object()),
                "Invalid sampling result metadata");
        const Json blocks = value["content"].is_array() ? value["content"] : Json::array({value["content"]});
        for (const auto& block : blocks) {
            require(block["type"] != "tool_result", "Sampling result cannot contain a tool result");
        }
    }

    const Json& SamplingResult::toJson() const {
        return value;
    }

    RootsResult::RootsResult(Json result)
        : value(std::move(result)) {
        require(value.is_object() && value.at("roots").is_array() && (!value.contains("_meta") || value["_meta"].is_object()),
                "Invalid roots result");
        std::set<std::string> uris;
        for (const auto& root : value["roots"]) {
            const auto uri = root.at("uri").get<std::string>();
            bool validUri = uri.starts_with("file://") && uri.find('/', 7) != std::string::npos;
            for (std::size_t index = 7; validUri && index < uri.size(); ++index) {
                const auto byte = static_cast<unsigned char>(uri[index]);
                validUri = byte > 32 && byte < 127 && byte != '?' && byte != '#';
                if (byte == '%') {
                    validUri = index + 2 < uri.size() && std::isxdigit(static_cast<unsigned char>(uri[index + 1])) != 0 &&
                               std::isxdigit(static_cast<unsigned char>(uri[index + 2])) != 0;
                    index += 2;
                }
            }
            require(validUri && uris.insert(uri).second && (!root.contains("name") || root["name"].is_string()) &&
                        (!root.contains("_meta") || root["_meta"].is_object()),
                    "Invalid or duplicate file root");
        }
    }

    const Json& RootsResult::toJson() const {
        return value;
    }
} // namespace mcp

namespace mcp::detail {
    using mcp::require;

    Json encodeInput(const ClientInput& input) {
        Json request;
        std::visit(
            [&](const auto& item) {
                using Input = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<Input, SamplingRequest>) {
                    request = {{"method", "sampling/createMessage"}, {"params", item.toJson()}};
                } else if constexpr (std::is_same_v<Input, RootsRequest>) {
                    request = {{"method", "roots/list"}, {"params", Json::object()}};
                } else {
                    Json params{{"mode", std::is_same_v<Input, FormInput> ? "form" : "url"}, {"message", item.message}};
                    if constexpr (std::is_same_v<Input, FormInput>) {
                        std::string error;
                        require(item.requestedSchema.is_object() && item.requestedSchema.value("type", Json()) == "object" &&
                                    item.requestedSchema.value("properties", Json()).is_object() &&
                                    Schema::check(item.requestedSchema, error),
                                "Invalid elicitation schema");
                        for (const auto& property : item.requestedSchema["properties"]) {
                            const Json type = property.is_object() ? property.value("type", Json()) : Json();
                            require(type == "string" || type == "number" || type == "integer" || type == "boolean" || type == "array",
                                    "Invalid elicitation property");
                        }
                        params["requestedSchema"] = item.requestedSchema;
                    } else {
                        require((item.url.starts_with("https://") || item.url.starts_with("http://")) && item.url.size() <= 16384,
                                "Invalid elicitation URL");
                        params["url"] = item.url;
                    }
                    request = {{"method", "elicitation/create"}, {"params", std::move(params)}};
                }
            },
            input);
        return request;
    }

    Json missingInputCapabilities(const Json& request, const Json& capabilities) {
        Json missing = Json::object();
        const auto method = request.at("method").get<std::string>();
        const Json params = request.value("params", Json::object());
        const auto category = method == "roots/list" ? "roots" : method == "sampling/createMessage" ? "sampling" : "elicitation";
        const Json advertised = capabilities.value(category, Json());
        Json required = Json::object();
        if (method == "sampling/createMessage") {
            bool tools = params.contains("tools") || params.contains("toolChoice");
            for (const auto& message : params.value("messages", Json::array())) {
                const Json blocks = message["content"].is_array() ? message["content"] : Json::array({message["content"]});
                for (const auto& block : blocks) {
                    tools = tools || block["type"] == "tool_use" || block["type"] == "tool_result";
                }
            }
            if (tools) {
                required["tools"] = Json::object();
            }
            if (params.value("includeContext", "none") != "none") {
                required["context"] = Json::object();
            }
        } else if (method == "elicitation/create") {
            const std::string mode = params.value("mode", "form");
            if (!(mode == "form" && advertised.is_object() && advertised.empty())) {
                required[mode] = Json::object();
            }
        }
        if (!advertised.is_object()) {
            missing[category] = required;
        } else {
            for (const auto& [key, value] : required.items()) {
                if (!advertised.contains(key) || !advertised[key].is_object()) {
                    missing[category][key] = value;
                }
            }
        }
        return missing;
    }

    bool validInputResult(const Json& request, const Json& result) {
        bool valid = false;
        try {
            const auto method = request.at("method").get<std::string>();
            if (method == "roots/list") {
                static_cast<void>(RootsResult(result));
                valid = true;
            } else if (method == "sampling/createMessage") {
                static_cast<void>(SamplingResult(result));
                valid = true;
                const Json params = request.value("params", Json::object());
                bool used = false;
                const Json blocks = result["content"].is_array() ? result["content"] : Json::array({result["content"]});
                for (const auto& block : blocks) {
                    if (block["type"] == "tool_use" && request.contains("params")) {
                        used = true;
                        bool found = false;
                        for (const auto& tool : params.value("tools", Json::array())) {
                            if (tool["name"] == block["name"]) {
                                std::string error;
                                found = Schema::validate(tool["inputSchema"], block["input"], error);
                            }
                        }
                        valid = valid && found && params.value("toolChoice", Json::object()).value("mode", "auto") != "none";
                    }
                }
                valid = valid && (used || params.value("toolChoice", Json::object()).value("mode", "auto") != "required");
            } else if (method == "elicitation/create") {
                const Json action = result.value("action", Json());
                const Json content = result.value("content", Json());
                const Json schema = request.value("params", Json::object()).value("requestedSchema", Json());
                std::string error;
                valid = (action == "accept" || action == "decline" || action == "cancel") && (content.is_null() || content.is_object()) &&
                        (action != "accept" || schema.is_null() || Schema::validate(schema, content, error));
            }
        } catch (const std::exception&) {
            valid = false;
        }
        return valid;
    }
} // namespace mcp::detail
