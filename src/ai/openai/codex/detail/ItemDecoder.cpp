/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ItemDecoder.h"

#include "ai/openai/codex/Protocol.h"

#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {

        const Json* findMember(const Json& object, const char* name) {
            if (!object.is_object()) {
                return nullptr;
            }

            const auto member = object.find(name);
            return member == object.end() ? nullptr : &*member;
        }

        bool requireString(const Json& object, const char* name, std::string& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                error = std::string("item is missing required '") + name + "' field";
                return false;
            }
            if (!member->is_string()) {
                error = std::string("item '") + name + "' field must be a string";
                return false;
            }

            value = member->get<std::string>();
            return true;
        }

        bool readOptionalString(const Json& object, const char* name, std::optional<std::string>& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr || member->is_null()) {
                value.reset();
                return true;
            }
            if (!member->is_string()) {
                error = std::string("item '") + name + "' field must be a string or null";
                return false;
            }

            value = member->get<std::string>();
            return true;
        }

        bool requireArray(const Json& object, const char* name, Json& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                error = std::string("item is missing required '") + name + "' field";
                return false;
            }
            if (!member->is_array()) {
                error = std::string("item '") + name + "' field must be an array";
                return false;
            }

            value = *member;
            return true;
        }

        bool readStringArray(const Json& object, const char* name, std::vector<std::string>& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                value.clear();
                return true;
            }
            if (!member->is_array()) {
                error = std::string("item '") + name + "' field must be an array";
                return false;
            }

            std::vector<std::string> decoded;
            decoded.reserve(member->size());
            for (const Json& entry : *member) {
                if (!entry.is_string()) {
                    error = std::string("item '") + name + "' entries must be strings";
                    return false;
                }
                decoded.push_back(entry.get<std::string>());
            }
            value = std::move(decoded);
            return true;
        }

        bool readOptionalInt32(const Json& object, const char* name, std::optional<std::int32_t>& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr || member->is_null()) {
                value.reset();
                return true;
            }

            if (member->is_number_unsigned()) {
                const std::uint64_t integer = member->get<std::uint64_t>();
                if (integer > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
                    error = std::string("item '") + name + "' field is outside the int32 range";
                    return false;
                }
                value = static_cast<std::int32_t>(integer);
                return true;
            }
            if (member->is_number_integer()) {
                const std::int64_t integer = member->get<std::int64_t>();
                if (integer < std::numeric_limits<std::int32_t>::min() || integer > std::numeric_limits<std::int32_t>::max()) {
                    error = std::string("item '") + name + "' field is outside the int32 range";
                    return false;
                }
                value = static_cast<std::int32_t>(integer);
                return true;
            }

            error = std::string("item '") + name + "' field must be an integer or null";
            return false;
        }

        bool readOptionalInt64(const Json& object, const char* name, std::optional<std::int64_t>& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr || member->is_null()) {
                value.reset();
                return true;
            }

            if (member->is_number_unsigned()) {
                const std::uint64_t integer = member->get<std::uint64_t>();
                if (integer > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    error = std::string("item '") + name + "' field is outside the int64 range";
                    return false;
                }
                value = static_cast<std::int64_t>(integer);
                return true;
            }
            if (member->is_number_integer()) {
                value = member->get<std::int64_t>();
                return true;
            }

            error = std::string("item '") + name + "' field must be an integer or null";
            return false;
        }

        bool makeMetadata(const Json& value,
                          const std::optional<typed::ThreadId>& threadId,
                          const std::optional<typed::TurnId>& turnId,
                          typed::ItemMetadata& metadata,
                          std::string& error) {
            std::string id;
            if (!requireString(value, "id", id, error)) {
                return false;
            }
            if (id.empty()) {
                error = "item 'id' field must be a non-empty string";
                return false;
            }

            metadata = typed::ItemMetadata{typed::ItemId{std::move(id)}, threadId, turnId, value};
            return true;
        }

        typed::UnknownItem makeUnknownItem(const Json& value,
                                           std::optional<std::string> type,
                                           const std::optional<typed::ThreadId>& threadId,
                                           const std::optional<typed::TurnId>& turnId,
                                           std::optional<std::string> decodingError = std::nullopt) {
            typed::UnknownItem item{std::move(type), value, std::move(decodingError), {}};
            item.metadata.threadId = threadId;
            item.metadata.turnId = turnId;

            const Json* id = findMember(value, "id");
            if (id == nullptr) {
                if (!item.decodingError) {
                    item.decodingError = "item is missing required 'id' field";
                }
            } else if (!id->is_string()) {
                if (!item.decodingError) {
                    item.decodingError = "item 'id' field must be a string";
                }
            } else {
                const std::string idValue = id->get<std::string>();
                if (idValue.empty()) {
                    if (!item.decodingError) {
                        item.decodingError = "item 'id' field must be a non-empty string";
                    }
                } else {
                    item.metadata.id = typed::ItemId{idValue};
                }
            }

            return item;
        }

        std::optional<typed::Item> decodeAgentMessage(const Json& value,
                                                      const std::optional<typed::ThreadId>& threadId,
                                                      const std::optional<typed::TurnId>& turnId,
                                                      std::string& error) {
            typed::AgentMessageItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, error) || !requireString(value, "text", item.text, error) ||
                !readOptionalString(value, "phase", item.phase, error)) {
                return std::nullopt;
            }
            return typed::Item{std::move(item)};
        }

        std::optional<typed::Item> decodeUserMessage(const Json& value,
                                                     const std::optional<typed::ThreadId>& threadId,
                                                     const std::optional<typed::TurnId>& turnId,
                                                     std::string& error) {
            typed::UserMessageItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, error) ||
                !readOptionalString(value, "clientId", item.clientId, error) || !requireArray(value, "content", item.content, error)) {
                return std::nullopt;
            }
            return typed::Item{std::move(item)};
        }

        std::optional<typed::Item> decodeReasoning(const Json& value,
                                                   const std::optional<typed::ThreadId>& threadId,
                                                   const std::optional<typed::TurnId>& turnId,
                                                   std::string& error) {
            typed::ReasoningItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, error) || !readStringArray(value, "summary", item.summary, error) ||
                !readStringArray(value, "content", item.content, error)) {
                return std::nullopt;
            }
            return typed::Item{std::move(item)};
        }

        std::optional<typed::Item> decodeCommandExecution(const Json& value,
                                                          const std::optional<typed::ThreadId>& threadId,
                                                          const std::optional<typed::TurnId>& turnId,
                                                          std::string& error) {
            typed::CommandExecutionItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, error) || !requireString(value, "command", item.command, error) ||
                !requireString(value, "cwd", item.cwd, error) || !requireString(value, "status", item.status, error) ||
                !requireArray(value, "commandActions", item.commandActions, error) ||
                !readOptionalString(value, "processId", item.processId, error) ||
                !readOptionalInt32(value, "exitCode", item.exitCode, error) ||
                !readOptionalInt64(value, "durationMs", item.durationMs, error) ||
                !readOptionalString(value, "aggregatedOutput", item.output, error)) {
                return std::nullopt;
            }
            return typed::Item{std::move(item)};
        }

        std::optional<typed::Item> decodeFileChange(const Json& value,
                                                    const std::optional<typed::ThreadId>& threadId,
                                                    const std::optional<typed::TurnId>& turnId,
                                                    std::string& error) {
            typed::FileChangeItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, error) || !requireArray(value, "changes", item.changes, error) ||
                !requireString(value, "status", item.status, error)) {
                return std::nullopt;
            }
            return typed::Item{std::move(item)};
        }

        std::optional<typed::Item> decodeMcpToolCall(const Json& value,
                                                     const std::optional<typed::ThreadId>& threadId,
                                                     const std::optional<typed::TurnId>& turnId,
                                                     std::string& error) {
            typed::ToolCallItem item;
            item.type = "mcpToolCall";
            const Json* arguments = findMember(value, "arguments");
            if (!makeMetadata(value, threadId, turnId, item.metadata, error) || !readOptionalString(value, "server", item.server, error) ||
                !requireString(value, "tool", item.tool, error) || !requireString(value, "status", item.status, error)) {
                return std::nullopt;
            }
            if (!item.server.has_value()) {
                error = "item is missing required 'server' field";
                return std::nullopt;
            }
            if (arguments == nullptr) {
                error = "item is missing required 'arguments' field";
                return std::nullopt;
            }

            item.arguments = *arguments;
            if (const Json* result = findMember(value, "result"); result != nullptr) {
                if (!result->is_null() && !result->is_object()) {
                    error = "item 'result' field must be an object or null";
                    return std::nullopt;
                }
                item.result = *result;
            }
            if (const Json* toolError = findMember(value, "error"); toolError != nullptr) {
                if (!toolError->is_null() && !toolError->is_object()) {
                    error = "item 'error' field must be an object or null";
                    return std::nullopt;
                }
                item.error = *toolError;
            }

            return typed::Item{std::move(item)};
        }

        std::optional<typed::Item> decodeDynamicToolCall(const Json& value,
                                                         const std::optional<typed::ThreadId>& threadId,
                                                         const std::optional<typed::TurnId>& turnId,
                                                         std::string& error) {
            typed::ToolCallItem item;
            item.type = "dynamicToolCall";
            const Json* arguments = findMember(value, "arguments");
            if (!makeMetadata(value, threadId, turnId, item.metadata, error) ||
                !readOptionalString(value, "namespace", item.nameSpace, error) || !requireString(value, "tool", item.tool, error) ||
                !requireString(value, "status", item.status, error)) {
                return std::nullopt;
            }
            if (arguments == nullptr) {
                error = "item is missing required 'arguments' field";
                return std::nullopt;
            }

            item.arguments = *arguments;
            if (const Json* contentItems = findMember(value, "contentItems"); contentItems != nullptr) {
                if (!contentItems->is_null() && !contentItems->is_array()) {
                    error = "item 'contentItems' field must be an array or null";
                    return std::nullopt;
                }
                item.result = *contentItems;
            }
            if (const Json* success = findMember(value, "success"); success != nullptr && !success->is_null() && !success->is_boolean()) {
                error = "item 'success' field must be a boolean or null";
                return std::nullopt;
            }

            return typed::Item{std::move(item)};
        }

        std::optional<typed::Item> decodeWebSearch(const Json& value,
                                                   const std::optional<typed::ThreadId>& threadId,
                                                   const std::optional<typed::TurnId>& turnId,
                                                   std::string& error) {
            typed::WebSearchItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, error) || !requireString(value, "query", item.query, error)) {
                return std::nullopt;
            }

            if (const Json* action = findMember(value, "action"); action != nullptr) {
                if (!action->is_null() && !action->is_object()) {
                    error = "item 'action' field must be an object or null";
                    return std::nullopt;
                }
                item.action = *action;
            }
            return typed::Item{std::move(item)};
        }

    } // namespace

    std::optional<typed::Item>
    decodeItem(const Json& value, std::optional<typed::ThreadId> threadId, std::optional<typed::TurnId> turnId, std::string& error) {
        try {
            error.clear();
            if (!value.is_object()) {
                error = "item must be an object";
                return std::nullopt;
            }

            const Json* discriminator = findMember(value, "type");
            if (discriminator == nullptr) {
                typed::UnknownItem item = makeUnknownItem(value, std::nullopt, threadId, turnId, "item is missing required 'type' field");
                return typed::Item{std::move(item)};
            }
            if (!discriminator->is_string()) {
                typed::UnknownItem item = makeUnknownItem(value, std::nullopt, threadId, turnId, "item 'type' field must be a string");
                return typed::Item{std::move(item)};
            }

            const std::string type = discriminator->get<std::string>();
            if (type.empty()) {
                typed::UnknownItem item =
                    makeUnknownItem(value, std::nullopt, threadId, turnId, "item 'type' field must be a non-empty string");
                return typed::Item{std::move(item)};
            }

            std::optional<typed::Item> decoded;
            if (type == "agentMessage") {
                decoded = decodeAgentMessage(value, threadId, turnId, error);
            } else if (type == "userMessage") {
                decoded = decodeUserMessage(value, threadId, turnId, error);
            } else if (type == "reasoning") {
                decoded = decodeReasoning(value, threadId, turnId, error);
            } else if (type == "commandExecution") {
                decoded = decodeCommandExecution(value, threadId, turnId, error);
            } else if (type == "fileChange") {
                decoded = decodeFileChange(value, threadId, turnId, error);
            } else if (type == "mcpToolCall") {
                decoded = decodeMcpToolCall(value, threadId, turnId, error);
            } else if (type == "dynamicToolCall") {
                decoded = decodeDynamicToolCall(value, threadId, turnId, error);
            } else if (type == "webSearch") {
                decoded = decodeWebSearch(value, threadId, turnId, error);
            } else {
                return typed::Item{makeUnknownItem(value, type, threadId, turnId)};
            }

            if (decoded) {
                return decoded;
            }

            typed::UnknownItem item = makeUnknownItem(value, type, threadId, turnId, error);
            error.clear();
            return typed::Item{std::move(item)};
        } catch (const std::exception& exception) {
            error = std::string("item decoding failed: ") + exception.what();
        } catch (...) {
            error = "item decoding failed with an unknown exception";
        }
        return std::nullopt;
    }

} // namespace ai::openai::codex::detail
