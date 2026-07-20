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
        const std::optional<Item> rejectedExitOverflow = decode(exitOverflow, error);
        testResult.expectTrue(!rejectedExitOverflow && !error.empty(), "commandExecution rejects exitCode outside int32 range");

        Json durationOverflow = optionalFieldsAbsent;
        durationOverflow["durationMs"] = std::numeric_limits<std::uint64_t>::max();
        const std::optional<Item> rejectedDurationOverflow = decode(durationOverflow, error);
        testResult.expectTrue(!rejectedDurationOverflow && !error.empty(), "commandExecution rejects durationMs outside int64 range");
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
            {"futurePayload", Json::array({1, false, "three"})},
        };
        std::string error;
        const std::optional<Item> decodedFuture = decode(futureRaw, error);
        const UnknownItem* future = as<UnknownItem>(decodedFuture);
        testResult.expectTrue(future && future->type == "futureItem" && future->raw == futureRaw && !future->decodingError,
                              "unknown item discriminator becomes UnknownItem with exact raw payload and no fatal error");

        const Json missingText = {{"type", "agentMessage"}, {"id", "bad-agent"}};
        const std::optional<Item> rejectedMissingText = decode(missingText, error);
        testResult.expectTrue(!rejectedMissingText && !error.empty(), "known item missing a required field reports a decoding error");

        const Json invalidId = {{"type", "reasoning"}, {"id", 7}};
        const std::optional<Item> rejectedInvalidId = decode(invalidId, error);
        testResult.expectTrue(!rejectedInvalidId && !error.empty(), "known item with non-string ID reports a decoding error");

        const Json missingType = {{"id", "missing-type"}};
        const std::optional<Item> rejectedMissingType = decode(missingType, error);
        testResult.expectTrue(!rejectedMissingType && !error.empty(), "item missing its discriminator reports a decoding error");

        const std::optional<Item> rejectedScalar = decode(Json::array({1, 2}), error);
        testResult.expectTrue(!rejectedScalar && !error.empty(), "non-object item reports a decoding error without throwing");
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    testAgentMessage(testResult);
    testReasoning(testResult);
    testCommandExecution(testResult);
    testFileChange(testResult);
    testToolCalls(testResult);
    testWebSearch(testResult);
    testUnknownAndMalformed(testResult);

    return testResult.processResult();
}
