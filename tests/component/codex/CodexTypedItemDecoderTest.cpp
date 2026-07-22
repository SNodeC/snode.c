/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ItemDecoder.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <variant>

namespace {
    using ai::openai::codex::Json;
    using ai::openai::codex::detail::decodeItem;
    using ai::openai::codex::typed::AgentMessageItem;
    using ai::openai::codex::typed::CommandExecutionItem;
    using ai::openai::codex::typed::FileChangeItem;
    using ai::openai::codex::typed::Item;
    using ai::openai::codex::typed::ReasoningItem;
    using ai::openai::codex::typed::ThreadId;
    using ai::openai::codex::typed::ToolCallItem;
    using ai::openai::codex::typed::TurnId;
    using ai::openai::codex::typed::UnknownItem;
    using ai::openai::codex::typed::UserMessageItem;
    using ai::openai::codex::typed::WebSearchItem;

    std::optional<Item> decode(const Json& raw, std::string& error) {
        return decodeItem(raw, ThreadId{"thread-typed"}, TurnId{"turn-typed"}, error);
    }

    template <typename T>
    const T* as(const std::optional<Item>& item) {
        return item ? std::get_if<T>(&*item) : nullptr;
    }

    void expectMetadata(tests::support::TestResult& testResult,
                        const ai::openai::codex::typed::ItemMetadata& metadata,
                        const std::string& id,
                        const Json& raw,
                        const std::string& description) {
        testResult.expectTrue(metadata.id.value == id && metadata.threadId && metadata.threadId->value == "thread-typed" &&
                                  metadata.turnId && metadata.turnId->value == "turn-typed",
                              description + " preserves typed item, thread, and turn identifiers");
        testResult.expectTrue(metadata.raw == raw, description + " preserves the exact raw item including future fields");
    }

    void expectUnknownMetadata(tests::support::TestResult& testResult,
                               const UnknownItem& item,
                               const std::optional<std::string>& id,
                               const Json& raw,
                               const std::string& description) {
        const bool idMatches = id.has_value() ? item.metadata.id && item.metadata.id->value == *id : !item.metadata.id;
        testResult.expectTrue(idMatches && item.metadata.threadId && item.metadata.threadId->value == "thread-typed" &&
                                  item.metadata.turnId && item.metadata.turnId->value == "turn-typed",
                              description + " preserves every usable common identifier");
        testResult.expectTrue(item.raw == raw, description + " preserves the exact raw item");
    }

    void testAgentMessage(tests::support::TestResult& testResult) {
        const Json raw = {
            {"type", "agentMessage"},
            {"id", "agent-1"},
            {"text", "Analysis complete."},
            {"phase", "future_phase"},
            {"futureAgentField", Json::array({1, "kept"})},
        };
        std::string error = "stale";
        const std::optional<Item> decoded = decode(raw, error);
        const AgentMessageItem* item = as<AgentMessageItem>(decoded);

        testResult.expectTrue(item != nullptr, "agentMessage item is classified");
        testResult.expectTrue(item && item->text == "Analysis complete." && item->phase == "future_phase",
                              "agentMessage preserves text and an unknown extensible phase string");
        testResult.expectTrue(error.empty(), "successful item decoding clears a stale error");
        if (item) {
            expectMetadata(testResult, item->metadata, "agent-1", raw, "agentMessage item");
        }

        const Json withoutPhase = {{"type", "agentMessage"}, {"id", "agent-2"}, {"text", "No phase"}};
        const std::optional<Item> decodedWithoutPhase = decode(withoutPhase, error);
        const AgentMessageItem* noPhase = as<AgentMessageItem>(decodedWithoutPhase);
        testResult.expectTrue(noPhase && !noPhase->phase, "agentMessage accepts an absent optional phase");
    }

    void testUserMessage(tests::support::TestResult& testResult) {
        const Json content = Json::array({
            Json{{"type", "text"}, {"text", "Answer just with OK!"}, {"text_elements", Json::array()}},
            Json{{"type", "image"}, {"url", "https://example.invalid/input.png"}, {"detail", nullptr}},
            Json{{"type", "futureInput"}, {"payload", Json{{"nested", Json::array({1, false, "kept"})}}}},
        });
        const Json raw = {
            {"type", "userMessage"},
            {"id", "user-message-1"},
            {"clientId", nullptr},
            {"content", content},
            {"futureUserMessageField", Json{{"kept", true}}},
        };
        std::string error = "stale";
        const std::optional<Item> decoded = decode(raw, error);
        const UserMessageItem* item = as<UserMessageItem>(decoded);

        testResult.expectTrue(item != nullptr, "userMessage item is classified");
        testResult.expectTrue(item && !item->clientId, "userMessage accepts a nullable clientId");
        testResult.expectTrue(item && item->content == content && item->content.size() == 3,
                              "userMessage preserves multiple content entries exactly and in order");
        testResult.expectTrue(item && item->content[0]["text"] == "Answer just with OK!" && item->content[2]["type"] == "futureInput" &&
                                  item->content[2]["payload"] == content[2]["payload"],
                              "userMessage preserves text and unfamiliar future content-entry variants losslessly");
        testResult.expectTrue(error.empty(), "successful userMessage decoding clears a stale external error");
        if (item) {
            expectMetadata(testResult, item->metadata, "user-message-1", raw, "userMessage item");
        }

        Json withClientId = raw;
        withClientId["id"] = "user-message-2";
        withClientId["clientId"] = "client-message-2";
        const std::optional<Item> decodedWithClientId = decode(withClientId, error);
        const UserMessageItem* clientItem = as<UserMessageItem>(decodedWithClientId);
        testResult.expectTrue(clientItem && clientItem->clientId == "client-message-2" && clientItem->content == content,
                              "userMessage preserves a non-null clientId without changing content");

        Json invalidContent = raw;
        invalidContent["id"] = "user-message-invalid-content";
        invalidContent["content"] = Json::object({{"text", "not an array"}});
        const std::optional<Item> decodedInvalidContent = decode(invalidContent, error);
        const UnknownItem* invalidContentItem = as<UnknownItem>(decodedInvalidContent);
        testResult.expectTrue(invalidContentItem && invalidContentItem->type == "userMessage" && invalidContentItem->decodingError &&
                                  error.empty(),
                              "malformed userMessage content degrades locally to a diagnosable UnknownItem");
        if (invalidContentItem) {
            expectUnknownMetadata(testResult,
                                  *invalidContentItem,
                                  std::string("user-message-invalid-content"),
                                  invalidContent,
                                  "malformed userMessage content");
        }

        Json invalidClientId = raw;
        invalidClientId["id"] = "user-message-invalid-client";
        invalidClientId["clientId"] = Json::array({"not", "a", "string"});
        const std::optional<Item> decodedInvalidClientId = decode(invalidClientId, error);
        const UnknownItem* invalidClientItem = as<UnknownItem>(decodedInvalidClientId);
        testResult.expectTrue(invalidClientItem && invalidClientItem->decodingError && error.empty(),
                              "malformed userMessage clientId retains common metadata in an UnknownItem");
        if (invalidClientItem) {
            expectUnknownMetadata(testResult,
                                  *invalidClientItem,
                                  std::string("user-message-invalid-client"),
                                  invalidClientId,
                                  "malformed userMessage clientId");
        }
    }

    void testReasoning(tests::support::TestResult& testResult) {
        const Json raw = {
            {"type", "reasoning"},
            {"id", "reasoning-1"},
            {"summary", Json::array({"first", "second"})},
            {"content", Json::array({"detail"})},
            {"futureReasoningField", true},
        };
        std::string error;
        const std::optional<Item> decoded = decode(raw, error);
        const ReasoningItem* item = as<ReasoningItem>(decoded);

        testResult.expectTrue(item && item->summary.size() == 2 && item->summary[0] == "first" && item->content.size() == 1 &&
                                  item->content[0] == "detail",
                              "reasoning item decodes summary and content arrays");
        if (item) {
            expectMetadata(testResult, item->metadata, "reasoning-1", raw, "reasoning item");
        }

        const Json defaults = {{"type", "reasoning"}, {"id", "reasoning-defaults"}};
        const std::optional<Item> decodedDefaults = decode(defaults, error);
        const ReasoningItem* defaultItem = as<ReasoningItem>(decodedDefaults);
        testResult.expectTrue(defaultItem && defaultItem->summary.empty() && defaultItem->content.empty(),
                              "reasoning applies schema defaults when optional arrays are absent");
    }

    void testCommandExecution(tests::support::TestResult& testResult) {
        const Json raw = {
            {"type", "commandExecution"},
            {"id", "command-1"},
            {"command", "printf hello"},
            {"cwd", "/tmp/project"},
            {"status", "future_status"},
            {"commandActions", Json::array({Json{{"type", "unknownFutureAction"}}})},
            {"processId", "pty-7"},
            {"exitCode", std::numeric_limits<std::int32_t>::min()},
            {"durationMs", std::numeric_limits<std::int64_t>::min()},
            {"aggregatedOutput", "hello"},
            {"futureCommandField", Json{{"preserved", true}}},
        };
        std::string error;
        const std::optional<Item> decoded = decode(raw, error);
        const CommandExecutionItem* item = as<CommandExecutionItem>(decoded);

        testResult.expectTrue(item && item->command == "printf hello" && item->cwd == "/tmp/project" && item->status == "future_status" &&
                                  item->commandActions == raw["commandActions"],
                              "commandExecution decodes required fields and preserves an extensible status");
        testResult.expectTrue(item && item->processId == "pty-7" && item->exitCode == std::numeric_limits<std::int32_t>::min() &&
                                  item->durationMs == std::numeric_limits<std::int64_t>::min() && item->output == "hello",
                              "commandExecution preserves signed integer boundaries and optional output fields");
        if (item) {
            expectMetadata(testResult, item->metadata, "command-1", raw, "commandExecution item");
        }

        const Json optionalFieldsAbsent = {
            {"type", "commandExecution"},
            {"id", "command-optional"},
            {"command", "true"},
            {"cwd", "/tmp"},
            {"status", "completed"},
            {"commandActions", Json::array()},
        };
        const std::optional<Item> decodedOptionalFields = decode(optionalFieldsAbsent, error);
        const CommandExecutionItem* optionalItem = as<CommandExecutionItem>(decodedOptionalFields);
        testResult.expectTrue(optionalItem && !optionalItem->processId && !optionalItem->exitCode && !optionalItem->durationMs &&
                                  !optionalItem->output,
                              "commandExecution accepts all optional result fields being absent");

        Json maximumValues = optionalFieldsAbsent;
        maximumValues["exitCode"] = std::numeric_limits<std::int32_t>::max();
        maximumValues["durationMs"] = std::numeric_limits<std::int64_t>::max();
        const std::optional<Item> decodedMaximumValues = decode(maximumValues, error);
        const CommandExecutionItem* maximumItem = as<CommandExecutionItem>(decodedMaximumValues);
        testResult.expectTrue(maximumItem && maximumItem->exitCode == std::numeric_limits<std::int32_t>::max() &&
                                  maximumItem->durationMs == std::numeric_limits<std::int64_t>::max(),
                              "commandExecution preserves maximum signed integer values");

        Json exitOverflow = optionalFieldsAbsent;
        exitOverflow["exitCode"] = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) + 1;
        const std::optional<Item> decodedExitOverflow = decode(exitOverflow, error);
        const UnknownItem* exitOverflowItem = as<UnknownItem>(decodedExitOverflow);
        testResult.expectTrue(exitOverflowItem && exitOverflowItem->decodingError && error.empty(),
                              "commandExecution with exitCode outside int32 degrades to a diagnosable UnknownItem");
        if (exitOverflowItem) {
            expectUnknownMetadata(
                testResult, *exitOverflowItem, std::string("command-optional"), exitOverflow, "commandExecution with overflowing exitCode");
        }

        Json durationOverflow = optionalFieldsAbsent;
        durationOverflow["durationMs"] = std::numeric_limits<std::uint64_t>::max();
        const std::optional<Item> decodedDurationOverflow = decode(durationOverflow, error);
        const UnknownItem* durationOverflowItem = as<UnknownItem>(decodedDurationOverflow);
        testResult.expectTrue(durationOverflowItem && durationOverflowItem->decodingError && error.empty(),
                              "commandExecution with durationMs outside int64 degrades to a diagnosable UnknownItem");
        if (durationOverflowItem) {
            expectUnknownMetadata(testResult,
                                  *durationOverflowItem,
                                  std::string("command-optional"),
                                  durationOverflow,
                                  "commandExecution with overflowing durationMs");
        }
    }

    void testFileChange(tests::support::TestResult& testResult) {
        const Json raw = {
            {"type", "fileChange"},
            {"id", "file-1"},
            {"changes", Json::array({Json{{"path", "/tmp/a"}, {"kind", "future"}}})},
            {"status", "declined"},
            {"futureFileField", 8},
        };
        std::string error;
        const std::optional<Item> decoded = decode(raw, error);
        const FileChangeItem* item = as<FileChangeItem>(decoded);

        testResult.expectTrue(item && item->changes == raw["changes"] && item->status == "declined",
                              "fileChange decodes changes and status without discarding nested extensions");
        if (item) {
            expectMetadata(testResult, item->metadata, "file-1", raw, "fileChange item");
        }
    }

    void testToolCalls(tests::support::TestResult& testResult) {
        const Json mcpRaw = {
            {"type", "mcpToolCall"},
            {"id", "mcp-1"},
            {"server", "files"},
            {"tool", "read"},
            {"status", "completed"},
            {"arguments", Json{{"path", "/tmp/a"}}},
            {"result", Json{{"content", Json::array({Json{{"type", "text"}, {"text", "ok"}}})}}},
            {"error", nullptr},
            {"futureMcpField", "kept"},
        };
        std::string error;
        const std::optional<Item> decodedMcp = decode(mcpRaw, error);
        const ToolCallItem* mcp = as<ToolCallItem>(decodedMcp);

        testResult.expectTrue(mcp && mcp->type == "mcpToolCall" && mcp->server == "files" && !mcp->nameSpace && mcp->tool == "read" &&
                                  mcp->arguments == mcpRaw["arguments"] && mcp->result == mcpRaw["result"] && mcp->error.is_null(),
                              "mcpToolCall decodes its discriminator, server, tool, arguments, result, and null error");
        if (mcp) {
            expectMetadata(testResult, mcp->metadata, "mcp-1", mcpRaw, "mcpToolCall item");
        }

        const Json dynamicRaw = {
            {"type", "dynamicToolCall"},
            {"id", "dynamic-1"},
            {"namespace", "workspace"},
            {"tool", "inspect"},
            {"status", "future_status"},
            {"arguments", Json::array({1, 2})},
            {"contentItems", Json::array({Json{{"type", "inputText"}, {"text", "done"}}})},
            {"success", false},
            {"futureDynamicField", true},
        };
        const std::optional<Item> decodedDynamic = decode(dynamicRaw, error);
        const ToolCallItem* dynamic = as<ToolCallItem>(decodedDynamic);
        testResult.expectTrue(dynamic && dynamic->type == "dynamicToolCall" && !dynamic->server && dynamic->nameSpace == "workspace" &&
                                  dynamic->tool == "inspect" && dynamic->status == "future_status" &&
                                  dynamic->arguments == dynamicRaw["arguments"] && dynamic->result == dynamicRaw["contentItems"],
                              "dynamicToolCall maps contentItems to the common raw result field and preserves extensible values");
        if (dynamic) {
            expectMetadata(testResult, dynamic->metadata, "dynamic-1", dynamicRaw, "dynamicToolCall item");
        }
    }

    void testWebSearch(tests::support::TestResult& testResult) {
        const Json raw = {
            {"type", "webSearch"},
            {"id", "web-1"},
            {"query", "Codex App Server schema"},
            {"action", Json{{"type", "futureSearchAction"}, {"detail", 3}}},
            {"futureWebField", nullptr},
        };
        std::string error;
        const std::optional<Item> decoded = decode(raw, error);
        const WebSearchItem* item = as<WebSearchItem>(decoded);

        testResult.expectTrue(item && item->query == "Codex App Server schema" && item->action == raw["action"],
                              "webSearch decodes query and preserves a future action object");
        if (item) {
            expectMetadata(testResult, item->metadata, "web-1", raw, "webSearch item");
        }

        const Json actionAbsent = {{"type", "webSearch"}, {"id", "web-2"}, {"query", "optional"}};
        const std::optional<Item> decodedActionAbsent = decode(actionAbsent, error);
        const WebSearchItem* noAction = as<WebSearchItem>(decodedActionAbsent);
        testResult.expectTrue(noAction && noAction->action.is_null(), "webSearch accepts an absent optional action");
    }

    void testUnknownAndMalformed(tests::support::TestResult& testResult) {
        const Json futureRaw = {
            {"type", "futureItem"},
            {"id", "future-item-1"},
            {"futurePayload", Json::array({1, false, "three"})},
        };
        std::string error = "stale";
        const std::optional<Item> decodedFuture = decode(futureRaw, error);
        const UnknownItem* future = as<UnknownItem>(decodedFuture);
        testResult.expectTrue(future && future->type == "futureItem" && !future->decodingError && error.empty(),
                              "unknown item discriminator becomes a nonfatal UnknownItem and clears the external error");
        if (future) {
            expectUnknownMetadata(testResult, *future, std::string("future-item-1"), futureRaw, "unknown future item");
        }

        const Json missingText = {{"type", "agentMessage"}, {"id", "bad-agent"}};
        const std::optional<Item> decodedMissingText = decode(missingText, error);
        const UnknownItem* missingTextItem = as<UnknownItem>(decodedMissingText);
        testResult.expectTrue(missingTextItem && missingTextItem->type == "agentMessage" && missingTextItem->decodingError && error.empty(),
                              "known item missing a detail field degrades to UnknownItem without a fatal external error");
        if (missingTextItem) {
            expectUnknownMetadata(testResult, *missingTextItem, std::string("bad-agent"), missingText, "agentMessage missing text");
        }

        const Json invalidId = {{"type", "reasoning"}, {"id", 7}};
        const std::optional<Item> decodedInvalidId = decode(invalidId, error);
        const UnknownItem* invalidIdItem = as<UnknownItem>(decodedInvalidId);
        testResult.expectTrue(invalidIdItem && invalidIdItem->type == "reasoning" && invalidIdItem->decodingError && error.empty(),
                              "known item with non-string ID retains its raw object and an item-local decoding error");
        if (invalidIdItem) {
            expectUnknownMetadata(testResult, *invalidIdItem, std::nullopt, invalidId, "item with a non-string ID");
        }

        const Json missingId = {{"type", "futureItem"}, {"futurePayload", true}};
        const std::optional<Item> decodedMissingId = decode(missingId, error);
        const UnknownItem* missingIdItem = as<UnknownItem>(decodedMissingId);
        testResult.expectTrue(missingIdItem && missingIdItem->decodingError && error.empty(),
                              "unknown item missing its ID retains a bounded item-local diagnostic");
        if (missingIdItem) {
            expectUnknownMetadata(testResult, *missingIdItem, std::nullopt, missingId, "unknown item missing its ID");
        }

        const Json emptyId = {{"type", "futureItem"}, {"id", ""}, {"futurePayload", false}};
        const std::optional<Item> decodedEmptyId = decode(emptyId, error);
        const UnknownItem* emptyIdItem = as<UnknownItem>(decodedEmptyId);
        testResult.expectTrue(emptyIdItem && !emptyIdItem->metadata.id && emptyIdItem->decodingError && error.empty(),
                              "unknown item with an empty ID does not fabricate a stable identifier");
        if (emptyIdItem) {
            expectUnknownMetadata(testResult, *emptyIdItem, std::nullopt, emptyId, "unknown item with an empty ID");
        }

        const Json missingType = {{"id", "missing-type"}};
        const std::optional<Item> decodedMissingType = decode(missingType, error);
        const UnknownItem* missingTypeItem = as<UnknownItem>(decodedMissingType);
        testResult.expectTrue(missingTypeItem && !missingTypeItem->type && missingTypeItem->decodingError && error.empty(),
                              "item missing its discriminator retains common metadata with an item-local error");
        if (missingTypeItem) {
            expectUnknownMetadata(testResult, *missingTypeItem, std::string("missing-type"), missingType, "item missing its discriminator");
        }

        const std::optional<Item> rejectedScalar = decode(Json::array({1, 2}), error);
        testResult.expectTrue(!rejectedScalar && !error.empty(), "non-object item reports a decoding error without throwing");
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    testAgentMessage(testResult);
    testUserMessage(testResult);
    testReasoning(testResult);
    testCommandExecution(testResult);
    testFileChange(testResult);
    testToolCalls(testResult);
    testWebSearch(testResult);
    testUnknownAndMalformed(testResult);

    return testResult.processResult();
}
