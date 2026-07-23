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
    using ai::openai::codex::typed::DecodeIssueKind;
    using ai::openai::codex::typed::DecodeIssueSeverity;
    using ai::openai::codex::typed::DynamicToolCallThreadItem;
    using ai::openai::codex::typed::FileChangeItem;
    using ai::openai::codex::typed::ImageUserInput;
    using ai::openai::codex::typed::InputTextDynamicToolCallOutputContentItem;
    using ai::openai::codex::typed::Item;
    using ai::openai::codex::typed::ReasoningItem;
    using ai::openai::codex::typed::TextUserInput;
    using ai::openai::codex::typed::ThreadId;
    using ai::openai::codex::typed::ToolCallItem;
    using ai::openai::codex::typed::TurnId;
    using ai::openai::codex::typed::UnknownItem;
    using ai::openai::codex::typed::UnknownPatchChangeKind;
    using ai::openai::codex::typed::UnknownUserInput;
    using ai::openai::codex::typed::UnknownWebSearchAction;
    using ai::openai::codex::typed::UnrecognizedCommandAction;
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

    bool isForwardCompatibility(const std::optional<ai::openai::codex::typed::DecodeDiagnostic>& diagnostic,
                                DecodeIssueKind kind,
                                const std::string& fieldPath,
                                const std::string& surface = "ThreadItem") {
        return diagnostic && diagnostic->kind == kind && diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility &&
               diagnostic->surface == surface && diagnostic->fieldPath == fieldPath;
    }

    bool isMalformedKnown(const std::optional<ai::openai::codex::typed::DecodeDiagnostic>& diagnostic,
                          const std::string& fieldPath) {
        return diagnostic && diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
               diagnostic->severity == DecodeIssueSeverity::ProtocolWarning && diagnostic->surface == "ThreadItem" &&
               diagnostic->fieldPath == fieldPath;
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
        testResult.expectTrue(item && item->text == "Analysis complete." && item->phase.hasValue() &&
                                  item->phase->value == "future_phase" && item->diagnostics.size() == 1 &&
                                  item->diagnostics.front().kind == DecodeIssueKind::UnknownEnumValue &&
                                  item->diagnostics.front().severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  item->diagnostics.front().surface == "ThreadItem" && item->diagnostics.front().fieldPath == "$.phase",
                              "agentMessage preserves an unknown open phase as a typed value with its exact forward diagnostic");
        testResult.expectTrue(error.empty(), "successful item decoding clears a stale error");
        if (item) {
            expectMetadata(testResult, item->metadata, "agent-1", raw, "agentMessage item");
        }

        const Json withoutPhase = {{"type", "agentMessage"}, {"id", "agent-2"}, {"text", "No phase"}};
        const std::optional<Item> decodedWithoutPhase = decode(withoutPhase, error);
        const AgentMessageItem* noPhase = as<AgentMessageItem>(decodedWithoutPhase);
        testResult.expectTrue(noPhase && noPhase->phase.isOmitted(), "agentMessage preserves an omitted optional phase");
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
        testResult.expectTrue(item && item->clientId.isNull(), "userMessage preserves an explicit-null clientId");
        const TextUserInput* text =
            item && item->content.size() == 3 ? std::get_if<TextUserInput>(&item->content[0]) : nullptr;
        const ImageUserInput* image =
            item && item->content.size() == 3 ? std::get_if<ImageUserInput>(&item->content[1]) : nullptr;
        const UnknownUserInput* future =
            item && item->content.size() == 3 ? std::get_if<UnknownUserInput>(&item->content[2]) : nullptr;
        testResult.expectTrue(text && text->text == "Answer just with OK!" && text->textElements &&
                                  text->textElements->empty() && text->raw == content[0] && image &&
                                  image->url == "https://example.invalid/input.png" && image->detail.isNull() &&
                                  image->raw == content[1],
                              "userMessage decodes known content entries into direction-specific typed alternatives in order");
        testResult.expectTrue(future && future->type == "futureInput" && future->raw == content[2] &&
                                  isForwardCompatibility(
                                      future->diagnostic, DecodeIssueKind::UnknownDiscriminator, "$.type", "UserInput") &&
                                  item->diagnostics.size() == 1 &&
                                  item->diagnostics.front().fieldPath == "$.content[2].type",
                              "userMessage preserves an unfamiliar content alternative losslessly with an exact relocated diagnostic");
        testResult.expectTrue(error.empty(), "successful userMessage decoding clears a stale external error");
        if (item) {
            expectMetadata(testResult, item->metadata, "user-message-1", raw, "userMessage item");
        }

        Json withClientId = raw;
        withClientId["id"] = "user-message-2";
        withClientId["clientId"] = "client-message-2";
        const std::optional<Item> decodedWithClientId = decode(withClientId, error);
        const UserMessageItem* clientItem = as<UserMessageItem>(decodedWithClientId);
        testResult.expectTrue(clientItem && clientItem->clientId.hasValue() &&
                                  clientItem->clientId->value == "client-message-2" && clientItem->content.size() == 3 &&
                                  std::get<UnknownUserInput>(clientItem->content[2]).raw == content[2],
                              "userMessage preserves a non-null clientId without changing typed or unknown content");

        Json invalidContent = raw;
        invalidContent["id"] = "user-message-invalid-content";
        invalidContent["content"] = Json::object({{"text", "not an array"}});
        const std::optional<Item> decodedInvalidContent = decode(invalidContent, error);
        const UnknownItem* invalidContentItem = as<UnknownItem>(decodedInvalidContent);
        testResult.expectTrue(invalidContentItem && invalidContentItem->type == "userMessage" &&
                                  invalidContentItem->decodingError &&
                                  isMalformedKnown(invalidContentItem->diagnostic, "$.content") && error.empty(),
                              "malformed userMessage content degrades locally with the exact malformed-known diagnostic");
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
        testResult.expectTrue(invalidClientItem && invalidClientItem->decodingError &&
                                  isMalformedKnown(invalidClientItem->diagnostic, "$.clientId") && error.empty(),
                              "malformed userMessage clientId retains metadata and the exact malformed-known diagnostic");
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

        testResult.expectTrue(item && item->summary && item->summary->size() == 2 && item->summary->at(0) == "first" &&
                                  item->content && item->content->size() == 1 && item->content->at(0) == "detail",
                              "reasoning item decodes summary and content arrays");
        if (item) {
            expectMetadata(testResult, item->metadata, "reasoning-1", raw, "reasoning item");
        }

        const Json defaults = {{"type", "reasoning"}, {"id", "reasoning-defaults"}};
        const std::optional<Item> decodedDefaults = decode(defaults, error);
        const ReasoningItem* defaultItem = as<ReasoningItem>(decodedDefaults);
        testResult.expectTrue(defaultItem && !defaultItem->summary && !defaultItem->content &&
                                  defaultItem->summaryOrDefault().empty() && defaultItem->contentOrDefault().empty(),
                              "reasoning preserves omitted optional arrays while its compatibility accessors remain empty");
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

        const UnrecognizedCommandAction* futureAction =
            item && item->commandActions.size() == 1 ? std::get_if<UnrecognizedCommandAction>(&item->commandActions.front()) : nullptr;
        testResult.expectTrue(item && item->command == "printf hello" && item->cwd.value == "/tmp/project" &&
                                  item->status.value == "future_status" && futureAction &&
                                  futureAction->type == "unknownFutureAction" && futureAction->raw == raw["commandActions"][0] &&
                                  futureAction->diagnostic &&
                                  futureAction->diagnostic->kind == DecodeIssueKind::UnknownDiscriminator &&
                                  futureAction->diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  futureAction->diagnostic->surface == "CommandAction" &&
                                  futureAction->diagnostic->fieldPath == "$.type",
                              "commandExecution decodes required fields and explicitly preserves a future command action");
        testResult.expectTrue(item && item->processId.hasValue() && *item->processId == "pty-7" &&
                                  item->exitCode.hasValue() &&
                                  *item->exitCode == std::numeric_limits<std::int32_t>::min() &&
                                  item->durationMs.hasValue() &&
                                  *item->durationMs == std::numeric_limits<std::int64_t>::min() &&
                                  item->aggregatedOutput.hasValue() && *item->aggregatedOutput == "hello",
                              "commandExecution preserves signed integer boundaries and tri-state optional output fields");
        testResult.expectTrue(item && item->diagnostics.size() == 2 &&
                                  item->diagnostics[0].kind == DecodeIssueKind::UnknownDiscriminator &&
                                  item->diagnostics[0].severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  item->diagnostics[0].surface == "CommandAction" &&
                                  item->diagnostics[0].fieldPath == "$.commandActions[0].type" &&
                                  item->diagnostics[1].kind == DecodeIssueKind::UnknownEnumValue &&
                                  item->diagnostics[1].severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  item->diagnostics[1].surface == "ThreadItem" &&
                                  item->diagnostics[1].fieldPath == "$.status",
                              "commandExecution reports the exact nested-discriminator and open-enum diagnostics");
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
        testResult.expectTrue(optionalItem && optionalItem->processId.isOmitted() && optionalItem->exitCode.isOmitted() &&
                                  optionalItem->durationMs.isOmitted() && optionalItem->aggregatedOutput.isOmitted() &&
                                  !optionalItem->source && optionalItem->sourceOrDefault().value == "agent",
                              "commandExecution preserves omitted result fields and its protocol default source");

        Json maximumValues = optionalFieldsAbsent;
        maximumValues["exitCode"] = std::numeric_limits<std::int32_t>::max();
        maximumValues["durationMs"] = std::numeric_limits<std::int64_t>::max();
        const std::optional<Item> decodedMaximumValues = decode(maximumValues, error);
        const CommandExecutionItem* maximumItem = as<CommandExecutionItem>(decodedMaximumValues);
        testResult.expectTrue(maximumItem && maximumItem->exitCode.hasValue() &&
                                  *maximumItem->exitCode == std::numeric_limits<std::int32_t>::max() &&
                                  maximumItem->durationMs.hasValue() &&
                                  *maximumItem->durationMs == std::numeric_limits<std::int64_t>::max(),
                              "commandExecution preserves maximum signed integer values");

        Json exitOverflow = optionalFieldsAbsent;
        exitOverflow["exitCode"] = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) + 1;
        const std::optional<Item> decodedExitOverflow = decode(exitOverflow, error);
        const UnknownItem* exitOverflowItem = as<UnknownItem>(decodedExitOverflow);
        testResult.expectTrue(exitOverflowItem && exitOverflowItem->decodingError &&
                                  isMalformedKnown(exitOverflowItem->diagnostic, "$.exitCode") && error.empty(),
                              "commandExecution with exitCode outside int32 degrades with the exact malformed-known diagnostic");
        if (exitOverflowItem) {
            expectUnknownMetadata(
                testResult, *exitOverflowItem, std::string("command-optional"), exitOverflow, "commandExecution with overflowing exitCode");
        }

        Json durationOverflow = optionalFieldsAbsent;
        durationOverflow["durationMs"] = std::numeric_limits<std::uint64_t>::max();
        const std::optional<Item> decodedDurationOverflow = decode(durationOverflow, error);
        const UnknownItem* durationOverflowItem = as<UnknownItem>(decodedDurationOverflow);
        testResult.expectTrue(durationOverflowItem && durationOverflowItem->decodingError &&
                                  isMalformedKnown(durationOverflowItem->diagnostic, "$.durationMs") && error.empty(),
                              "commandExecution with durationMs outside int64 degrades with the exact malformed-known diagnostic");
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
            {"changes",
             Json::array(
                 {Json{{"path", "/tmp/a"}, {"diff", "@@ future patch @@"}, {"kind", Json{{"type", "futurePatchKind"}}}}})},
            {"status", "declined"},
            {"futureFileField", 8},
        };
        std::string error;
        const std::optional<Item> decoded = decode(raw, error);
        const FileChangeItem* item = as<FileChangeItem>(decoded);

        const UnknownPatchChangeKind* futureKind =
            item && item->changes.size() == 1 ? std::get_if<UnknownPatchChangeKind>(&item->changes.front().kind) : nullptr;
        testResult.expectTrue(item && item->changes.size() == 1 && item->changes.front().path == "/tmp/a" &&
                                  item->changes.front().diff == "@@ future patch @@" && futureKind &&
                                  futureKind->type == "futurePatchKind" && futureKind->raw == raw["changes"][0]["kind"] &&
                                  item->status.value == "declined" && item->diagnostics.size() == 1 &&
                                  item->diagnostics.front().kind == DecodeIssueKind::UnknownDiscriminator &&
                                  item->diagnostics.front().severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  item->diagnostics.front().surface == "PatchChangeKind" &&
                                  item->diagnostics.front().fieldPath == "$.changes[0].kind.type",
                              "fileChange types every known field while preserving a future patch-kind extension");
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

        testResult.expectTrue(mcp && mcp->server == "files" && mcp->tool == "read" &&
                                  mcp->status.value == "completed" && mcp->arguments &&
                                  *mcp->arguments == mcpRaw["arguments"] && mcp->result.hasValue() &&
                                  mcp->result->content.size() == 1 &&
                                  mcp->result->content.front() == mcpRaw["result"]["content"][0] &&
                                  mcp->result->meta.isOmitted() && mcp->result->structuredContent.isOmitted() &&
                                  mcp->error.isNull(),
                              "mcpToolCall decodes typed server/status/result fields, protocol-opaque arguments, and null error");
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
        const DynamicToolCallThreadItem* dynamic = as<DynamicToolCallThreadItem>(decodedDynamic);
        const InputTextDynamicToolCallOutputContentItem* dynamicText =
            dynamic && dynamic->contentItems.hasValue() && dynamic->contentItems->size() == 1
                ? std::get_if<InputTextDynamicToolCallOutputContentItem>(&dynamic->contentItems->front())
                : nullptr;
        testResult.expectTrue(dynamic && dynamic->nameSpace.hasValue() && *dynamic->nameSpace == "workspace" &&
                                  dynamic->tool == "inspect" && dynamic->status.value == "future_status" &&
                                  dynamic->arguments && *dynamic->arguments == dynamicRaw["arguments"] && dynamicText &&
                                  dynamicText->text == "done" && dynamicText->raw == dynamicRaw["contentItems"][0] &&
                                  dynamic->success.hasValue() && !*dynamic->success,
                              "dynamicToolCall remains directionally distinct and types contentItems without losing opaque arguments");
        testResult.expectTrue(dynamic && dynamic->diagnostics.size() == 1 &&
                                  dynamic->diagnostics.front().kind == DecodeIssueKind::UnknownEnumValue &&
                                  dynamic->diagnostics.front().severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  dynamic->diagnostics.front().surface == "ThreadItem" &&
                                  dynamic->diagnostics.front().fieldPath == "$.status",
                              "dynamicToolCall preserves a future open status with the exact diagnostic");
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

        const UnknownWebSearchAction* futureAction =
            item && item->action.hasValue() ? std::get_if<UnknownWebSearchAction>(&*item->action) : nullptr;
        testResult.expectTrue(item && item->query == "Codex App Server schema" && futureAction &&
                                  futureAction->type == "futureSearchAction" && futureAction->raw == raw["action"] &&
                                  futureAction->diagnostic &&
                                  futureAction->diagnostic->kind == DecodeIssueKind::UnknownDiscriminator &&
                                  futureAction->diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  futureAction->diagnostic->surface == "WebSearchAction" &&
                                  futureAction->diagnostic->fieldPath == "$.type" &&
                                  item->diagnostics.size() == 1 &&
                                  item->diagnostics.front().kind == DecodeIssueKind::UnknownDiscriminator &&
                                  item->diagnostics.front().severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  item->diagnostics.front().surface == "WebSearchAction" &&
                                  item->diagnostics.front().fieldPath == "$.action.type",
                              "webSearch decodes query and explicitly preserves a future action object");
        if (item) {
            expectMetadata(testResult, item->metadata, "web-1", raw, "webSearch item");
        }

        const Json actionAbsent = {{"type", "webSearch"}, {"id", "web-2"}, {"query", "optional"}};
        const std::optional<Item> decodedActionAbsent = decode(actionAbsent, error);
        const WebSearchItem* noAction = as<WebSearchItem>(decodedActionAbsent);
        testResult.expectTrue(noAction && noAction->action.isOmitted(), "webSearch preserves an omitted optional action");
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
        testResult.expectTrue(future && future->type == "futureItem" && !future->decodingError &&
                                  isForwardCompatibility(future->diagnostic, DecodeIssueKind::UnknownDiscriminator, "$.type") &&
                                  error.empty(),
                              "unknown item discriminator becomes a nonfatal UnknownItem with the exact forward diagnostic");
        if (future) {
            expectUnknownMetadata(testResult, *future, std::string("future-item-1"), futureRaw, "unknown future item");
        }

        const Json missingText = {{"type", "agentMessage"}, {"id", "bad-agent"}};
        const std::optional<Item> decodedMissingText = decode(missingText, error);
        const UnknownItem* missingTextItem = as<UnknownItem>(decodedMissingText);
        testResult.expectTrue(missingTextItem && missingTextItem->type == "agentMessage" && missingTextItem->decodingError &&
                                  isMalformedKnown(missingTextItem->diagnostic, "$.text") && error.empty(),
                              "known item missing a detail field degrades with the exact malformed-known diagnostic");
        if (missingTextItem) {
            expectUnknownMetadata(testResult, *missingTextItem, std::string("bad-agent"), missingText, "agentMessage missing text");
        }

        const Json invalidId = {{"type", "reasoning"}, {"id", 7}};
        const std::optional<Item> decodedInvalidId = decode(invalidId, error);
        const UnknownItem* invalidIdItem = as<UnknownItem>(decodedInvalidId);
        testResult.expectTrue(invalidIdItem && invalidIdItem->type == "reasoning" && invalidIdItem->decodingError &&
                                  isMalformedKnown(invalidIdItem->diagnostic, "$.id") && error.empty(),
                              "known item with non-string ID retains raw data and the exact malformed-known diagnostic");
        if (invalidIdItem) {
            expectUnknownMetadata(testResult, *invalidIdItem, std::nullopt, invalidId, "item with a non-string ID");
        }

        const Json missingId = {{"type", "futureItem"}, {"futurePayload", true}};
        const std::optional<Item> decodedMissingId = decode(missingId, error);
        const UnknownItem* missingIdItem = as<UnknownItem>(decodedMissingId);
        testResult.expectTrue(missingIdItem && missingIdItem->decodingError &&
                                  isMalformedKnown(missingIdItem->diagnostic, "$.id") && error.empty(),
                              "unknown item missing its ID retains the exact bounded malformed-known diagnostic");
        if (missingIdItem) {
            expectUnknownMetadata(testResult, *missingIdItem, std::nullopt, missingId, "unknown item missing its ID");
        }

        const Json emptyId = {{"type", "futureItem"}, {"id", ""}, {"futurePayload", false}};
        const std::optional<Item> decodedEmptyId = decode(emptyId, error);
        const UnknownItem* emptyIdItem = as<UnknownItem>(decodedEmptyId);
        testResult.expectTrue(emptyIdItem && !emptyIdItem->metadata.id && emptyIdItem->decodingError &&
                                  isMalformedKnown(emptyIdItem->diagnostic, "$.id") && error.empty(),
                              "unknown item with an empty ID does not fabricate an identifier and reports its exact path");
        if (emptyIdItem) {
            expectUnknownMetadata(testResult, *emptyIdItem, std::nullopt, emptyId, "unknown item with an empty ID");
        }

        const Json missingType = {{"id", "missing-type"}};
        const std::optional<Item> decodedMissingType = decode(missingType, error);
        const UnknownItem* missingTypeItem = as<UnknownItem>(decodedMissingType);
        testResult.expectTrue(missingTypeItem && !missingTypeItem->type && missingTypeItem->decodingError &&
                                  isMalformedKnown(missingTypeItem->diagnostic, "$.type") && error.empty(),
                              "item missing its discriminator retains metadata with the exact malformed-known diagnostic");
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
