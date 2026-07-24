/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ItemDecoder.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "support/TestResult.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#ifndef CODEX_A1_FIXTURE_ROOT
#error CODEX_A1_FIXTURE_ROOT must name the checked-in Codex fixture corpus
#endif

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    using codex::Json;

    const std::filesystem::path fixtureRoot{CODEX_A1_FIXTURE_ROOT};

    Json readJson(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input) {
            throw std::runtime_error("cannot open fixture: " + path.string());
        }
        return Json::parse(input);
    }

    Json fixture(std::string_view relativePath) {
        return readJson(fixtureRoot / relativePath);
    }

    template <typename T, typename Matcher>
    bool nullableMatches(const typed::OptionalNullable<T>& decoded, const Json& raw, std::string_view field, Matcher matcher) {
        const auto it = raw.find(field);
        if (it == raw.end()) {
            return decoded.isOmitted();
        }
        if (it->is_null()) {
            return decoded.isNull();
        }
        return decoded.hasValue() && matcher(*decoded.value, *it);
    }

    template <typename T>
    bool nullableScalarMatches(const typed::OptionalNullable<T>& decoded, const Json& raw, std::string_view field) {
        return nullableMatches(decoded, raw, field, [](const T& value, const Json& expected) {
            return value == expected.get<T>();
        });
    }

    template <typename T>
    bool nullableStrongStringMatches(const typed::OptionalNullable<T>& decoded, const Json& raw, std::string_view field) {
        return nullableMatches(decoded, raw, field, [](const T& value, const Json& expected) {
            return value.value == expected.get<std::string>();
        });
    }

    template <typename T>
    bool nullableOpenStringMatches(const typed::OptionalNullable<T>& decoded, const Json& raw, std::string_view field) {
        return nullableStrongStringMatches(decoded, raw, field);
    }

    bool requiredNullableJsonMatches(const std::optional<Json>& decoded, const Json& raw, std::string_view field) {
        const auto it = raw.find(field);
        return it != raw.end() && (it->is_null() ? !decoded.has_value() : decoded && *decoded == *it);
    }

    template <typename Union>
    const Json& unionRaw(const Union& value) {
        return std::visit(
            [](const auto& alternative) -> const Json& {
                return alternative.raw;
            },
            value);
    }

    template <typename Union>
    bool knownUnion(const Union& value) {
        return value.index() + 1 < std::variant_size_v<Union>;
    }

    template <typename Union>
    bool unionVectorMatches(const std::vector<Union>& decoded, const Json& raw) {
        if (!raw.is_array() || decoded.size() != raw.size()) {
            return false;
        }
        for (std::size_t index = 0; index < decoded.size(); ++index) {
            if (!knownUnion(decoded[index]) || unionRaw(decoded[index]) != raw[index]) {
                return false;
            }
        }
        return true;
    }

    template <typename Union>
    bool nullableUnionMatches(const typed::OptionalNullable<Union>& decoded, const Json& raw, std::string_view field) {
        return nullableMatches(decoded, raw, field, [](const Union& value, const Json& expected) {
            return knownUnion(value) && unionRaw(value) == expected;
        });
    }

    template <typename Union>
    bool nullableUnionVectorMatches(const typed::OptionalNullable<std::vector<Union>>& decoded, const Json& raw, std::string_view field) {
        return nullableMatches(decoded, raw, field, [](const std::vector<Union>& value, const Json& expected) {
            return unionVectorMatches(value, expected);
        });
    }

    template <typename Item>
    const std::vector<typed::DecodeDiagnostic>* diagnosticsFor(const Item& item) {
        return std::visit(
            [](const auto& alternative) -> const std::vector<typed::DecodeDiagnostic>* {
                if constexpr (requires { alternative.diagnostics; }) {
                    return &alternative.diagnostics;
                } else {
                    return nullptr;
                }
            },
            item);
    }

    bool metadataMatches(const typed::ItemMetadata& metadata, const Json& raw) {
        return metadata.id.value == raw.at("id").get<std::string>() && metadata.threadId &&
               metadata.threadId->value == "thread-a1-1-fixture" && metadata.turnId && metadata.turnId->value == "turn-a1-1-fixture" &&
               metadata.raw == raw;
    }

    bool memoryCitationMatches(const typed::MemoryCitation& decoded, const Json& raw) {
        if (!raw.is_object() || !raw.at("entries").is_array() || !raw.at("threadIds").is_array() ||
            decoded.entries.size() != raw.at("entries").size() || decoded.threadIds.size() != raw.at("threadIds").size()) {
            return false;
        }
        for (std::size_t index = 0; index < decoded.entries.size(); ++index) {
            const Json& expected = raw.at("entries")[index];
            const auto& actual = decoded.entries[index];
            if (actual.lineEnd != expected.at("lineEnd").get<std::uint32_t>() ||
                actual.lineStart != expected.at("lineStart").get<std::uint32_t>() ||
                actual.note != expected.at("note").get<std::string>() || actual.path != expected.at("path").get<std::string>()) {
                return false;
            }
        }
        for (std::size_t index = 0; index < decoded.threadIds.size(); ++index) {
            if (decoded.threadIds[index].value != raw.at("threadIds")[index].get<std::string>()) {
                return false;
            }
        }
        return true;
    }

    bool collabStatesMatch(const std::map<std::string, typed::CollabAgentState>& decoded, const Json& raw) {
        if (!raw.is_object() || decoded.size() != raw.size()) {
            return false;
        }
        for (const auto& [name, state] : decoded) {
            const auto expected = raw.find(name);
            if (expected == raw.end() || state.status.value != expected->at("status").get<std::string>() ||
                !nullableScalarMatches(state.message, *expected, "message")) {
                return false;
            }
        }
        return true;
    }

    bool fileChangesMatch(const std::vector<typed::FileUpdateChange>& decoded, const Json& raw) {
        if (!raw.is_array() || decoded.size() != raw.size()) {
            return false;
        }
        for (std::size_t index = 0; index < decoded.size(); ++index) {
            const Json& expected = raw[index];
            if (decoded[index].diff != expected.at("diff").get<std::string>() ||
                decoded[index].path != expected.at("path").get<std::string>() || !knownUnion(decoded[index].kind) ||
                unionRaw(decoded[index].kind) != expected.at("kind")) {
                return false;
            }
        }
        return true;
    }

    bool hookFragmentsMatch(const std::vector<typed::HookPromptFragment>& decoded, const Json& raw) {
        if (!raw.is_array() || decoded.size() != raw.size()) {
            return false;
        }
        for (std::size_t index = 0; index < decoded.size(); ++index) {
            if (decoded[index].hookRunId != raw[index].at("hookRunId").get<std::string>() ||
                decoded[index].text != raw[index].at("text").get<std::string>()) {
                return false;
            }
        }
        return true;
    }

    bool mcpAppContextMatches(const typed::McpToolCallAppContext& decoded, const Json& raw) {
        return decoded.connectorId == raw.at("connectorId").get<std::string>() &&
               nullableScalarMatches(decoded.actionName, raw, "actionName") && nullableScalarMatches(decoded.appName, raw, "appName") &&
               nullableScalarMatches(decoded.linkId, raw, "linkId") && nullableScalarMatches(decoded.resourceUri, raw, "resourceUri") &&
               nullableScalarMatches(decoded.templateId, raw, "templateId");
    }

    bool mcpResultMatches(const typed::McpToolCallResult& decoded, const Json& raw) {
        return decoded.content == raw.at("content").get<std::vector<Json>>() && nullableScalarMatches(decoded.meta, raw, "_meta") &&
               nullableScalarMatches(decoded.structuredContent, raw, "structuredContent");
    }

    bool threadFieldsMatch(const typed::ThreadItem& decoded, std::string_view expectedType, const Json& raw) {
        if (expectedType == "agentMessage") {
            const auto* item = std::get_if<typed::AgentMessageThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && item->text == raw.at("text").get<std::string>() &&
                   nullableMatches(item->memoryCitation, raw, "memoryCitation", memoryCitationMatches) &&
                   nullableOpenStringMatches(item->phase, raw, "phase");
        }
        if (expectedType == "collabAgentToolCall") {
            const auto* item = std::get_if<typed::CollabAgentToolCallThreadItem>(&decoded);
            if (!item || !metadataMatches(item->metadata, raw) || !collabStatesMatch(item->agentsStates, raw.at("agentsStates")) ||
                item->receiverThreadIds.size() != raw.at("receiverThreadIds").size() ||
                item->senderThreadId.value != raw.at("senderThreadId").get<std::string>() ||
                item->status.value != raw.at("status").get<std::string>() || item->tool.value != raw.at("tool").get<std::string>() ||
                !nullableStrongStringMatches(item->model, raw, "model") || !nullableScalarMatches(item->prompt, raw, "prompt") ||
                !nullableOpenStringMatches(item->reasoningEffort, raw, "reasoningEffort")) {
                return false;
            }
            for (std::size_t index = 0; index < item->receiverThreadIds.size(); ++index) {
                if (item->receiverThreadIds[index].value != raw.at("receiverThreadIds")[index].get<std::string>()) {
                    return false;
                }
            }
            return true;
        }
        if (expectedType == "commandExecution") {
            const auto* item = std::get_if<typed::CommandExecutionThreadItem>(&decoded);
            const auto source = raw.find("source");
            const bool sourceMatches =
                source == raw.end() ? !item || !item->source : item && item->source && item->source->value == source->get<std::string>();
            return item && metadataMatches(item->metadata, raw) && item->command == raw.at("command").get<std::string>() &&
                   unionVectorMatches(item->commandActions, raw.at("commandActions")) &&
                   item->cwd.value == raw.at("cwd").get<std::string>() && item->status.value == raw.at("status").get<std::string>() &&
                   nullableScalarMatches(item->aggregatedOutput, raw, "aggregatedOutput") &&
                   nullableScalarMatches(item->durationMs, raw, "durationMs") && nullableScalarMatches(item->exitCode, raw, "exitCode") &&
                   nullableScalarMatches(item->processId, raw, "processId") && sourceMatches;
        }
        if (expectedType == "contextCompaction") {
            const auto* item = std::get_if<typed::ContextCompactionThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw);
        }
        if (expectedType == "dynamicToolCall") {
            const auto* item = std::get_if<typed::DynamicToolCallThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && requiredNullableJsonMatches(item->arguments, raw, "arguments") &&
                   item->status.value == raw.at("status").get<std::string>() && item->tool == raw.at("tool").get<std::string>() &&
                   nullableUnionVectorMatches(item->contentItems, raw, "contentItems") &&
                   nullableScalarMatches(item->durationMs, raw, "durationMs") && nullableScalarMatches(item->nameSpace, raw, "namespace") &&
                   nullableScalarMatches(item->success, raw, "success");
        }
        if (expectedType == "enteredReviewMode") {
            const auto* item = std::get_if<typed::EnteredReviewModeThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && item->review == raw.at("review").get<std::string>();
        }
        if (expectedType == "exitedReviewMode") {
            const auto* item = std::get_if<typed::ExitedReviewModeThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && item->review == raw.at("review").get<std::string>();
        }
        if (expectedType == "fileChange") {
            const auto* item = std::get_if<typed::FileChangeThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && fileChangesMatch(item->changes, raw.at("changes")) &&
                   item->status.value == raw.at("status").get<std::string>();
        }
        if (expectedType == "hookPrompt") {
            const auto* item = std::get_if<typed::HookPromptThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && hookFragmentsMatch(item->fragments, raw.at("fragments"));
        }
        if (expectedType == "imageGeneration") {
            const auto* item = std::get_if<typed::ImageGenerationThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && item->result == raw.at("result").get<std::string>() &&
                   item->status == raw.at("status").get<std::string>() &&
                   nullableScalarMatches(item->revisedPrompt, raw, "revisedPrompt") &&
                   nullableStrongStringMatches(item->savedPath, raw, "savedPath");
        }
        if (expectedType == "imageView") {
            const auto* item = std::get_if<typed::ImageViewThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && item->path.value == raw.at("path").get<std::string>();
        }
        if (expectedType == "mcpToolCall") {
            const auto* item = std::get_if<typed::McpToolCallThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && requiredNullableJsonMatches(item->arguments, raw, "arguments") &&
                   item->server == raw.at("server").get<std::string>() && item->status.value == raw.at("status").get<std::string>() &&
                   item->tool == raw.at("tool").get<std::string>() &&
                   nullableMatches(item->appContext, raw, "appContext", mcpAppContextMatches) &&
                   nullableScalarMatches(item->durationMs, raw, "durationMs") &&
                   nullableMatches(item->error,
                                   raw,
                                   "error",
                                   [](const typed::McpToolCallError& value, const Json& expected) {
                                       return value.message == expected.at("message").get<std::string>();
                                   }) &&
                   nullableScalarMatches(item->mcpAppResourceUri, raw, "mcpAppResourceUri") &&
                   nullableScalarMatches(item->pluginId, raw, "pluginId") && nullableMatches(item->result, raw, "result", mcpResultMatches);
        }
        if (expectedType == "plan") {
            const auto* item = std::get_if<typed::PlanThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && item->text == raw.at("text").get<std::string>();
        }
        if (expectedType == "reasoning") {
            const auto* item = std::get_if<typed::ReasoningThreadItem>(&decoded);
            const auto content = raw.find("content");
            const auto summary = raw.find("summary");
            return item && metadataMatches(item->metadata, raw) &&
                   (content == raw.end() ? !item->content : item->content && *item->content == content->get<std::vector<std::string>>()) &&
                   (summary == raw.end() ? !item->summary : item->summary && *item->summary == summary->get<std::vector<std::string>>());
        }
        if (expectedType == "sleep") {
            const auto* item = std::get_if<typed::SleepThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && item->durationMs == raw.at("durationMs").get<std::uint64_t>();
        }
        if (expectedType == "subAgentActivity") {
            const auto* item = std::get_if<typed::SubAgentActivityThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && item->agentPath == raw.at("agentPath").get<std::string>() &&
                   item->agentThreadId.value == raw.at("agentThreadId").get<std::string>() &&
                   item->kind.value == raw.at("kind").get<std::string>();
        }
        if (expectedType == "userMessage") {
            const auto* item = std::get_if<typed::UserMessageThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && unionVectorMatches(item->content, raw.at("content")) &&
                   nullableStrongStringMatches(item->clientId, raw, "clientId");
        }
        if (expectedType == "webSearch") {
            const auto* item = std::get_if<typed::WebSearchThreadItem>(&decoded);
            return item && metadataMatches(item->metadata, raw) && item->query == raw.at("query").get<std::string>() &&
                   nullableUnionMatches(item->action, raw, "action");
        }
        return false;
    }

    bool internalMetadataMatches(const typed::InternalChatMessageMetadataPassthrough& decoded, const Json& raw) {
        return nullableStrongStringMatches(decoded.turnId, raw, "turn_id");
    }

    template <typename Item>
    bool responseCommonMatches(const Item& item, const Json& raw) {
        return nullableStrongStringMatches(item.id, raw, "id") &&
               nullableMatches(
                   item.internalChatMessageMetadataPassthrough, raw, "internal_chat_message_metadata_passthrough", internalMetadataMatches);
    }

    bool outputBodyMatches(const typed::FunctionCallOutputBody& decoded, const Json& raw) {
        if (raw.is_string()) {
            const auto* text = std::get_if<std::string>(&decoded);
            return text && *text == raw.get<std::string>();
        }
        const auto* content = std::get_if<std::vector<typed::FunctionCallOutputContentItem>>(&decoded);
        return content && unionVectorMatches(*content, raw);
    }

    bool responseFieldsMatch(const typed::ResponseItem& decoded, std::string_view expectedType, const Json& raw) {
        if (expectedType == "agent_message") {
            const auto* item = std::get_if<typed::AgentMessageResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && item->author == raw.at("author").get<std::string>() &&
                   unionVectorMatches(item->content, raw.at("content")) && item->recipient == raw.at("recipient").get<std::string>();
        }
        if (expectedType == "compaction") {
            const auto* item = std::get_if<typed::CompactionResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && item->encryptedContent == raw.at("encrypted_content").get<std::string>();
        }
        if (expectedType == "compaction_trigger") {
            const auto* item = std::get_if<typed::CompactionTriggerResponseItem>(&decoded);
            return item && item->raw == raw;
        }
        if (expectedType == "context_compaction") {
            const auto* item = std::get_if<typed::ContextCompactionResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && nullableScalarMatches(item->encryptedContent, raw, "encrypted_content");
        }
        if (expectedType == "custom_tool_call") {
            const auto* item = std::get_if<typed::CustomToolCallResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && item->callId.value == raw.at("call_id").get<std::string>() &&
                   item->input == raw.at("input").get<std::string>() && item->name == raw.at("name").get<std::string>() &&
                   nullableScalarMatches(item->nameSpace, raw, "namespace") && nullableScalarMatches(item->status, raw, "status");
        }
        if (expectedType == "custom_tool_call_output") {
            const auto* item = std::get_if<typed::CustomToolCallOutputResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && item->callId.value == raw.at("call_id").get<std::string>() &&
                   outputBodyMatches(item->output, raw.at("output")) && nullableScalarMatches(item->name, raw, "name");
        }
        if (expectedType == "function_call") {
            const auto* item = std::get_if<typed::FunctionCallResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && item->arguments == raw.at("arguments").get<std::string>() &&
                   item->callId.value == raw.at("call_id").get<std::string>() && item->name == raw.at("name").get<std::string>() &&
                   nullableScalarMatches(item->nameSpace, raw, "namespace");
        }
        if (expectedType == "function_call_output") {
            const auto* item = std::get_if<typed::FunctionCallOutputResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && item->callId.value == raw.at("call_id").get<std::string>() &&
                   outputBodyMatches(item->output, raw.at("output"));
        }
        if (expectedType == "image_generation_call") {
            const auto* item = std::get_if<typed::ImageGenerationCallResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && item->result == raw.at("result").get<std::string>() &&
                   item->status == raw.at("status").get<std::string>() && nullableScalarMatches(item->revisedPrompt, raw, "revised_prompt");
        }
        if (expectedType == "local_shell_call") {
            const auto* item = std::get_if<typed::LocalShellCallResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && knownUnion(item->action) && unionRaw(item->action) == raw.at("action") &&
                   item->status.value == raw.at("status").get<std::string>() && nullableStrongStringMatches(item->callId, raw, "call_id");
        }
        if (expectedType == "message") {
            const auto* item = std::get_if<typed::MessageResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && unionVectorMatches(item->content, raw.at("content")) &&
                   item->role == raw.at("role").get<std::string>() && nullableOpenStringMatches(item->phase, raw, "phase");
        }
        if (expectedType == "other") {
            const auto* item = std::get_if<typed::OtherResponseItem>(&decoded);
            return item && item->raw == raw;
        }
        if (expectedType == "reasoning") {
            const auto* item = std::get_if<typed::ReasoningResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && unionVectorMatches(item->summary, raw.at("summary")) &&
                   nullableUnionVectorMatches(item->content, raw, "content") &&
                   nullableScalarMatches(item->encryptedContent, raw, "encrypted_content");
        }
        if (expectedType == "tool_search_call") {
            const auto* item = std::get_if<typed::ToolSearchCallResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && requiredNullableJsonMatches(item->arguments, raw, "arguments") &&
                   item->execution == raw.at("execution").get<std::string>() && nullableStrongStringMatches(item->callId, raw, "call_id") &&
                   nullableScalarMatches(item->status, raw, "status");
        }
        if (expectedType == "tool_search_output") {
            const auto* item = std::get_if<typed::ToolSearchOutputResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && item->execution == raw.at("execution").get<std::string>() &&
                   item->status == raw.at("status").get<std::string>() && item->tools == raw.at("tools").get<std::vector<Json>>() &&
                   nullableStrongStringMatches(item->callId, raw, "call_id");
        }
        if (expectedType == "web_search_call") {
            const auto* item = std::get_if<typed::WebSearchCallResponseItem>(&decoded);
            return item && responseCommonMatches(*item, raw) && nullableUnionMatches(item->action, raw, "action") &&
                   nullableScalarMatches(item->status, raw, "status");
        }
        return false;
    }

    struct ThreadTargetExpectation {
        std::string_view name;
        detail::ItemDiscriminatorTarget target;
    };

    constexpr std::array<ThreadTargetExpectation, 18> threadTargets{{
        {"agentMessage", detail::ItemDiscriminatorTarget::AgentMessage},
        {"collabAgentToolCall", detail::ItemDiscriminatorTarget::CollabAgentToolCall},
        {"commandExecution", detail::ItemDiscriminatorTarget::CommandExecution},
        {"contextCompaction", detail::ItemDiscriminatorTarget::ContextCompaction},
        {"dynamicToolCall", detail::ItemDiscriminatorTarget::DynamicToolCall},
        {"enteredReviewMode", detail::ItemDiscriminatorTarget::EnteredReviewMode},
        {"exitedReviewMode", detail::ItemDiscriminatorTarget::ExitedReviewMode},
        {"fileChange", detail::ItemDiscriminatorTarget::FileChange},
        {"hookPrompt", detail::ItemDiscriminatorTarget::HookPrompt},
        {"imageGeneration", detail::ItemDiscriminatorTarget::ImageGeneration},
        {"imageView", detail::ItemDiscriminatorTarget::ImageView},
        {"mcpToolCall", detail::ItemDiscriminatorTarget::McpToolCall},
        {"plan", detail::ItemDiscriminatorTarget::Plan},
        {"reasoning", detail::ItemDiscriminatorTarget::Reasoning},
        {"sleep", detail::ItemDiscriminatorTarget::Sleep},
        {"subAgentActivity", detail::ItemDiscriminatorTarget::SubAgentActivity},
        {"userMessage", detail::ItemDiscriminatorTarget::UserMessage},
        {"webSearch", detail::ItemDiscriminatorTarget::WebSearch},
    }};

    struct ResponseTargetExpectation {
        std::string_view name;
        detail::ResponseItemTarget target;
    };

    constexpr std::array<ResponseTargetExpectation, 16> responseTargets{{
        {"agent_message", detail::ResponseItemTarget::AgentMessage},
        {"compaction", detail::ResponseItemTarget::Compaction},
        {"compaction_trigger", detail::ResponseItemTarget::CompactionTrigger},
        {"context_compaction", detail::ResponseItemTarget::ContextCompaction},
        {"custom_tool_call", detail::ResponseItemTarget::CustomToolCall},
        {"custom_tool_call_output", detail::ResponseItemTarget::CustomToolCallOutput},
        {"function_call", detail::ResponseItemTarget::FunctionCall},
        {"function_call_output", detail::ResponseItemTarget::FunctionCallOutput},
        {"image_generation_call", detail::ResponseItemTarget::ImageGenerationCall},
        {"local_shell_call", detail::ResponseItemTarget::LocalShellCall},
        {"message", detail::ResponseItemTarget::Message},
        {"other", detail::ResponseItemTarget::Other},
        {"reasoning", detail::ResponseItemTarget::Reasoning},
        {"tool_search_call", detail::ResponseItemTarget::ToolSearchCall},
        {"tool_search_output", detail::ResponseItemTarget::ToolSearchOutput},
        {"web_search_call", detail::ResponseItemTarget::WebSearchCall},
    }};

    bool diagnosticIs(const std::optional<typed::DecodeDiagnostic>& diagnostic,
                      typed::DecodeIssueKind kind,
                      typed::DecodeIssueSeverity severity,
                      std::string_view surface) {
        return diagnostic && diagnostic->kind == kind && diagnostic->severity == severity && diagnostic->surface == surface &&
               !diagnostic->fieldPath.empty();
    }

    void testRegistryAndDescriptors(tests::support::TestResult& result) {
        const auto threadDescriptors = detail::threadItemCodecDescriptors();
        const auto responseDescriptors = detail::responseItemCodecDescriptors();
        bool threadExact = threadDescriptors.size() == threadTargets.size();
        bool responseExact = responseDescriptors.size() == responseTargets.size();

        for (std::size_t index = 0; threadExact && index < threadTargets.size(); ++index) {
            const auto& descriptor = threadDescriptors[index];
            const auto& expected = threadTargets[index];
            const auto& row = detail::entryFor(expected.target);
            threadExact = descriptor.key.category == detail::SurfaceCategory::ItemDiscriminator && descriptor.key.domain == "ThreadItem" &&
                          descriptor.key.field == "type" && descriptor.key.name == expected.name && descriptor.target == expected.target &&
                          descriptor.shape == detail::ConversationUnionCodecShape::InternallyTaggedObject &&
                          descriptor.direction == detail::ConversationUnionCodecDirection::DecodeOnly && row.key == descriptor.key &&
                          std::get_if<detail::ItemDiscriminatorTarget>(&row.runtimeTarget) &&
                          *std::get_if<detail::ItemDiscriminatorTarget>(&row.runtimeTarget) == expected.target;
        }
        for (std::size_t index = 0; responseExact && index < responseTargets.size(); ++index) {
            const auto& descriptor = responseDescriptors[index];
            const auto& expected = responseTargets[index];
            const auto& row = detail::entryFor(expected.target);
            responseExact = descriptor.key.category == detail::SurfaceCategory::ItemDiscriminator &&
                            descriptor.key.domain == "ResponseItem" && descriptor.key.field == "type" &&
                            descriptor.key.name == expected.name && descriptor.target == expected.target &&
                            descriptor.shape == detail::ConversationUnionCodecShape::InternallyTaggedObject &&
                            descriptor.direction == detail::ConversationUnionCodecDirection::DecodeOnly && row.key == descriptor.key &&
                            std::get_if<detail::ResponseItemTarget>(&row.runtimeTarget) &&
                            *std::get_if<detail::ResponseItemTarget>(&row.runtimeTarget) == expected.target;
        }

        result.expectTrue(threadExact, "all 18 ThreadItem codecs have exact canonical registry-backed descriptors");
        result.expectTrue(responseExact, "all 16 ResponseItem codecs have exact canonical registry-backed descriptors");
    }

    void testIndexedCorpus(tests::support::TestResult& result) {
        const Json index = fixture("index.json");
        std::size_t positiveCount = 0;
        std::size_t malformedCount = 0;
        std::size_t futureEnumCount = 0;
        bool positivesExact = true;
        bool malformedExact = true;
        bool futureEnumsExact = true;

        for (const Json& entry : index.at("fixtures")) {
            const auto keyMember = entry.find("protocol_surface_key");
            if (keyMember == entry.end() || !keyMember->is_object()) {
                continue;
            }
            const Json& key = *keyMember;
            const std::string domain = key.at("domain").get<std::string>();
            if (domain != "ThreadItem" && domain != "ResponseItem") {
                continue;
            }
            const std::string role = entry.at("role").get<std::string>();
            const std::string expectedType = key.at("name").get<std::string>();
            const Json raw = fixture(entry.at("file").get<std::string>());
            const bool positive = role == "union_branch" || role == "union_branch_supplement" || role == "union_nullable_null" ||
                                  role == "union_optional_omitted";
            const bool malformed = role.starts_with("malformed_known_");

            if (domain == "ThreadItem") {
                std::string error = "stale";
                const auto decoded =
                    detail::decodeItem(raw, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
                if (positive) {
                    ++positiveCount;
                    positivesExact = positivesExact && decoded && error.empty() && threadFieldsMatch(*decoded, expectedType, raw) &&
                                     diagnosticsFor(*decoded) && diagnosticsFor(*decoded)->empty();
                } else if (malformed) {
                    ++malformedCount;
                    const auto* unknown = decoded ? std::get_if<typed::UnknownItem>(&*decoded) : nullptr;
                    malformedExact = malformedExact && unknown && error.empty() && unknown->raw == raw && unknown->decodingError &&
                                     diagnosticIs(unknown->diagnostic,
                                                  typed::DecodeIssueKind::MalformedKnownPayload,
                                                  typed::DecodeIssueSeverity::ProtocolWarning,
                                                  "ThreadItem");
                } else if (role == "unknown_enum_value") {
                    ++futureEnumCount;
                    const auto* diagnostics = decoded ? diagnosticsFor(*decoded) : nullptr;
                    futureEnumsExact = futureEnumsExact && decoded && error.empty() && threadFieldsMatch(*decoded, expectedType, raw) &&
                                       diagnostics && diagnostics->size() == 1 &&
                                       diagnostics->front().kind == typed::DecodeIssueKind::UnknownEnumValue &&
                                       diagnostics->front().severity == typed::DecodeIssueSeverity::ForwardCompatibility &&
                                       diagnostics->front().surface == "ThreadItem" && !diagnostics->front().fieldPath.empty();
                }
            } else {
                std::string error = "stale";
                const auto decoded = detail::decodeResponseItem(raw, error);
                if (positive) {
                    ++positiveCount;
                    positivesExact = positivesExact && decoded && error.empty() && responseFieldsMatch(*decoded, expectedType, raw) &&
                                     diagnosticsFor(*decoded) && diagnosticsFor(*decoded)->empty();
                } else if (malformed) {
                    ++malformedCount;
                    const auto* unknown = decoded ? std::get_if<typed::UnknownResponseItem>(&*decoded) : nullptr;
                    malformedExact = malformedExact && unknown && error.empty() && unknown->raw == raw && unknown->decodingError &&
                                     diagnosticIs(unknown->diagnostic,
                                                  typed::DecodeIssueKind::MalformedKnownPayload,
                                                  typed::DecodeIssueSeverity::ProtocolWarning,
                                                  "ResponseItem");
                } else if (role == "unknown_enum_value") {
                    ++futureEnumCount;
                    const auto* diagnostics = decoded ? diagnosticsFor(*decoded) : nullptr;
                    futureEnumsExact = futureEnumsExact && decoded && error.empty() && responseFieldsMatch(*decoded, expectedType, raw) &&
                                       diagnostics && diagnostics->size() == 1 &&
                                       diagnostics->front().kind == typed::DecodeIssueKind::UnknownEnumValue &&
                                       diagnostics->front().severity == typed::DecodeIssueSeverity::ForwardCompatibility &&
                                       diagnostics->front().surface == "ResponseItem" && !diagnostics->front().fieldPath.empty();
                }
            }
        }

        result.expectTrue(positiveCount == 259 && positivesExact,
                          "all 259 indexed positive, nullable, omitted, nested-alternative, and alternate-body item fixtures map every "
                          "represented field");
        result.expectTrue(malformedCount == 413 && malformedExact,
                          "all 413 indexed malformed-known item fixtures produce exactly MalformedKnownPayload/ProtocolWarning");
        result.expectTrue(futureEnumCount == 24 && futureEnumsExact,
                          "all 24 future and empty open-enum fixtures retain their typed outer alternative and exact forward diagnostic");
    }

    void testFutureOuterAlternatives(tests::support::TestResult& result) {
        const Json threadRaw = fixture("cases/unions/threaditem/future-unknown.json");
        const Json responseRaw = fixture("cases/unions/responseitem/future-unknown.json");
        std::string threadError = "stale";
        std::string responseError = "stale";
        const auto thread =
            detail::decodeItem(threadRaw, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, threadError);
        const auto response = detail::decodeResponseItem(responseRaw, responseError);
        const auto* unknownThread = thread ? std::get_if<typed::UnknownItem>(&*thread) : nullptr;
        const auto* unknownResponse = response ? std::get_if<typed::UnknownResponseItem>(&*response) : nullptr;

        result.expectTrue(
            unknownThread && threadError.empty() && unknownThread->type == "futureThreadItem" && unknownThread->raw == threadRaw &&
                unknownThread->metadata.id && unknownThread->metadata.id->value == threadRaw.at("id").get<std::string>() &&
                unknownThread->metadata.threadId && unknownThread->metadata.threadId->value == "thread-a1-1-fixture" &&
                unknownThread->metadata.turnId && unknownThread->metadata.turnId->value == "turn-a1-1-fixture" &&
                diagnosticIs(unknownThread->diagnostic,
                             typed::DecodeIssueKind::UnknownDiscriminator,
                             typed::DecodeIssueSeverity::ForwardCompatibility,
                             "ThreadItem"),
            "a future ThreadItem discriminator produces the direction-specific unknown alternative with raw and common metadata");
        result.expectTrue(unknownResponse && responseError.empty() && unknownResponse->type == "future_response_item" &&
                              unknownResponse->raw == responseRaw && unknownResponse->metadata.id &&
                              unknownResponse->metadata.id->value == responseRaw.at("id").get<std::string>() &&
                              diagnosticIs(unknownResponse->diagnostic,
                                           typed::DecodeIssueKind::UnknownDiscriminator,
                                           typed::DecodeIssueSeverity::ForwardCompatibility,
                                           "ResponseItem"),
                          "a future ResponseItem discriminator produces its distinct unknown alternative with raw and common metadata");
    }

    void testSensitiveDiagnosticOmission(tests::support::TestResult& result) {
        constexpr std::string_view secret = "codex-a1-1-secret-must-not-leak";
        Json threadRaw = fixture("cases/unions/threaditem/agentmessage.json");
        Json responseRaw = fixture("cases/unions/responseitem/agent_message.json");
        threadRaw["text"] = Json::array({secret});
        responseRaw["content"] = Json::object({{"secret", secret}});

        std::string threadError;
        std::string responseError;
        const auto thread =
            detail::decodeItem(threadRaw, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, threadError);
        const auto response = detail::decodeResponseItem(responseRaw, responseError);
        const auto* unknownThread = thread ? std::get_if<typed::UnknownItem>(&*thread) : nullptr;
        const auto* unknownResponse = response ? std::get_if<typed::UnknownResponseItem>(&*response) : nullptr;

        result.expectTrue(unknownThread && unknownThread->diagnostic &&
                              unknownThread->diagnostic->message.find(secret) == std::string::npos &&
                              unknownThread->diagnostic->fieldPath == "$.text",
                          "ThreadItem malformed diagnostics identify the field path without leaking its value");
        result.expectTrue(unknownResponse && unknownResponse->diagnostic &&
                              unknownResponse->diagnostic->message.find(secret) == std::string::npos &&
                              unknownResponse->diagnostic->fieldPath == "$.content",
                          "ResponseItem malformed diagnostics identify the field path without leaking its value");
    }

    void testIntegerBoundsAndLegacyReasoningOrder(tests::support::TestResult& result) {
        const auto malformedCommandField = [](const Json& raw, std::string_view path) {
            std::string decodeError;
            const auto decoded =
                detail::decodeItem(raw, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, decodeError);
            const auto* unknown = decoded ? std::get_if<typed::UnknownItem>(&*decoded) : nullptr;
            return unknown && unknown->diagnostic &&
                   unknown->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                   unknown->diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning &&
                   unknown->diagnostic->fieldPath == path;
        };

        Json command = fixture("cases/unions/threaditem/commandexecution.json");
        std::string error;
        command["exitCode"] = std::numeric_limits<std::int32_t>::min();
        const auto minimumExit =
            detail::decodeItem(command, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* minimumExitItem = minimumExit ? std::get_if<typed::CommandExecutionThreadItem>(&*minimumExit) : nullptr;
        command["exitCode"] = std::numeric_limits<std::int32_t>::max();
        const auto maximumExit =
            detail::decodeItem(command, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* maximumExitItem = maximumExit ? std::get_if<typed::CommandExecutionThreadItem>(&*maximumExit) : nullptr;
        result.expectTrue(minimumExitItem && minimumExitItem->exitCode.hasValue() &&
                              *minimumExitItem->exitCode == std::numeric_limits<std::int32_t>::min() && maximumExitItem &&
                              maximumExitItem->exitCode.hasValue() &&
                              *maximumExitItem->exitCode == std::numeric_limits<std::int32_t>::max(),
                          "CommandExecutionThreadItem accepts the exact pinned int32 exit-code bounds");

        command["exitCode"] = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) + 1;
        const bool upperExitOverflow = malformedCommandField(command, "$.exitCode");
        command["exitCode"] = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()) - 1;
        const bool lowerExitOverflow = malformedCommandField(command, "$.exitCode");
        command["exitCode"] = 1.5;
        const bool fractionalExit = malformedCommandField(command, "$.exitCode");
        result.expectTrue(upperExitOverflow && lowerExitOverflow && fractionalExit,
                          "CommandExecutionThreadItem rejects int32 overflow and fractional exit codes at the exact field path");

        command = fixture("cases/unions/threaditem/commandexecution.json");
        command["durationMs"] = std::numeric_limits<std::int64_t>::min();
        const auto minimumDuration =
            detail::decodeItem(command, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* minimumDurationItem =
            minimumDuration ? std::get_if<typed::CommandExecutionThreadItem>(&*minimumDuration) : nullptr;
        command["durationMs"] = std::numeric_limits<std::int64_t>::max();
        const auto maximumDuration =
            detail::decodeItem(command, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* maximumDurationItem =
            maximumDuration ? std::get_if<typed::CommandExecutionThreadItem>(&*maximumDuration) : nullptr;
        result.expectTrue(minimumDurationItem && minimumDurationItem->durationMs.hasValue() &&
                              *minimumDurationItem->durationMs == std::numeric_limits<std::int64_t>::min() &&
                              maximumDurationItem && maximumDurationItem->durationMs.hasValue() &&
                              *maximumDurationItem->durationMs == std::numeric_limits<std::int64_t>::max(),
                          "CommandExecutionThreadItem accepts the exact pinned int64 duration bounds");

        command["durationMs"] = std::numeric_limits<std::uint64_t>::max();
        const bool unsignedDurationOverflow = malformedCommandField(command, "$.durationMs");
        command["durationMs"] = 1.5;
        const bool fractionalDuration = malformedCommandField(command, "$.durationMs");
        result.expectTrue(unsignedDurationOverflow && fractionalDuration,
                          "CommandExecutionThreadItem rejects unsigned int64 overflow and fractional durations at the exact field path");

        Json citation = fixture("cases/unions/threaditem/agentmessage.json");
        citation["memoryCitation"]["entries"][0]["lineEnd"] = 0;
        const auto zero =
            detail::decodeItem(citation, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* zeroItem = zero ? std::get_if<typed::AgentMessageThreadItem>(&*zero) : nullptr;
        result.expectTrue(zeroItem && zeroItem->memoryCitation.hasValue() &&
                              zeroItem->memoryCitation->entries.front().lineEnd == 0,
                          "MemoryCitationEntry accepts the exact pinned uint32 lower bound");

        citation["memoryCitation"]["entries"][0]["lineEnd"] =
            std::numeric_limits<std::uint32_t>::max();
        const auto boundary =
            detail::decodeItem(citation, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* boundaryItem = boundary ? std::get_if<typed::AgentMessageThreadItem>(&*boundary) : nullptr;
        result.expectTrue(boundaryItem && boundaryItem->memoryCitation.hasValue() &&
                              boundaryItem->memoryCitation->entries.front().lineEnd == std::numeric_limits<std::uint32_t>::max(),
                          "MemoryCitationEntry accepts the exact pinned uint32 upper bound");

        citation["memoryCitation"]["entries"][0]["lineEnd"] =
            static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1U;
        const auto overflow =
            detail::decodeItem(citation, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* overflowItem = overflow ? std::get_if<typed::UnknownItem>(&*overflow) : nullptr;
        result.expectTrue(overflowItem && overflowItem->diagnostic &&
                              overflowItem->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              overflowItem->diagnostic->fieldPath == "$.memoryCitation.entries[0].lineEnd",
                          "MemoryCitationEntry rejects uint32 overflow at the exact field path");

        citation["memoryCitation"]["entries"][0]["lineEnd"] = -1;
        const auto negative =
            detail::decodeItem(citation, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* negativeItem = negative ? std::get_if<typed::UnknownItem>(&*negative) : nullptr;
        result.expectTrue(negativeItem && negativeItem->diagnostic &&
                              negativeItem->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              negativeItem->diagnostic->fieldPath == "$.memoryCitation.entries[0].lineEnd",
                          "MemoryCitationEntry rejects negative integers at the exact field path");

        citation["memoryCitation"]["entries"][0]["lineEnd"] = 1.5;
        const auto fractional =
            detail::decodeItem(citation, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* fractionalItem = fractional ? std::get_if<typed::UnknownItem>(&*fractional) : nullptr;
        result.expectTrue(fractionalItem && fractionalItem->diagnostic &&
                              fractionalItem->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              fractionalItem->diagnostic->fieldPath == "$.memoryCitation.entries[0].lineEnd",
                          "MemoryCitationEntry rejects fractional numbers at the exact field path");

        typed::ReasoningThreadItem legacyAggregate{
            typed::ItemMetadata{},
            std::vector<std::string>{"legacy-summary"},
            std::vector<std::string>{"legacy-content"},
        };
        result.expectTrue(legacyAggregate.summary && legacyAggregate.summary->front() == "legacy-summary" &&
                              legacyAggregate.content && legacyAggregate.content->front() == "legacy-content",
                          "ReasoningThreadItem preserves the legacy aggregate order metadata, summary, content");

        Json sleep = fixture("cases/unions/threaditem/sleep.json");
        sleep["durationMs"] = std::numeric_limits<std::uint64_t>::max();
        const auto maximumSleep =
            detail::decodeItem(sleep, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* maximumSleepItem = maximumSleep ? std::get_if<typed::SleepThreadItem>(&*maximumSleep) : nullptr;
        result.expectTrue(maximumSleepItem && maximumSleepItem->durationMs == std::numeric_limits<std::uint64_t>::max(),
                          "SleepThreadItem accepts the exact uint64 upper bound");

        sleep["durationMs"] = -1;
        const auto negativeSleep =
            detail::decodeItem(sleep, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* negativeSleepItem = negativeSleep ? std::get_if<typed::UnknownItem>(&*negativeSleep) : nullptr;
        sleep["durationMs"] = 1.5;
        const auto fractionalSleep =
            detail::decodeItem(sleep, typed::ThreadId{"thread-a1-1-fixture"}, typed::TurnId{"turn-a1-1-fixture"}, error);
        const auto* fractionalSleepItem = fractionalSleep ? std::get_if<typed::UnknownItem>(&*fractionalSleep) : nullptr;
        result.expectTrue(negativeSleepItem && negativeSleepItem->diagnostic &&
                              negativeSleepItem->diagnostic->fieldPath == "$.durationMs" && fractionalSleepItem &&
                              fractionalSleepItem->diagnostic && fractionalSleepItem->diagnostic->fieldPath == "$.durationMs",
                          "SleepThreadItem rejects negative and fractional uint64 values at the exact field path");
    }

    template <typename OpenEnum>
    bool exactKnownValues(std::initializer_list<std::pair<OpenEnum, std::string_view>> values,
                          std::size_t& knownValueCount) {
        for (const auto& [value, expected] : values) {
            ++knownValueCount;
            if (value.value != expected || !value.isKnown()) {
                return false;
            }
        }
        return !OpenEnum{"future_value"}.isKnown() && !OpenEnum{""}.isKnown();
    }

    void testB3OpenEnumFactories(tests::support::TestResult& result) {
        std::size_t knownValueCount = 0;
        const bool exact =
            exactKnownValues<typed::MessagePhase>(
                {
                    {typed::MessagePhase::commentary(), "commentary"},
                    {typed::MessagePhase::finalAnswer(), "final_answer"},
                },
                knownValueCount) &&
            exactKnownValues<typed::CollabAgentStatus>(
                {
                    {typed::CollabAgentStatus::pendingInit(), "pendingInit"},
                    {typed::CollabAgentStatus::running(), "running"},
                    {typed::CollabAgentStatus::interrupted(), "interrupted"},
                    {typed::CollabAgentStatus::completed(), "completed"},
                    {typed::CollabAgentStatus::errored(), "errored"},
                    {typed::CollabAgentStatus::shutdown(), "shutdown"},
                    {typed::CollabAgentStatus::notFound(), "notFound"},
                },
                knownValueCount) &&
            exactKnownValues<typed::CollabAgentTool>(
                {
                    {typed::CollabAgentTool::spawnAgent(), "spawnAgent"},
                    {typed::CollabAgentTool::sendInput(), "sendInput"},
                    {typed::CollabAgentTool::resumeAgent(), "resumeAgent"},
                    {typed::CollabAgentTool::wait(), "wait"},
                    {typed::CollabAgentTool::closeAgent(), "closeAgent"},
                },
                knownValueCount) &&
            exactKnownValues<typed::CollabAgentToolCallStatus>(
                {
                    {typed::CollabAgentToolCallStatus::inProgress(), "inProgress"},
                    {typed::CollabAgentToolCallStatus::completed(), "completed"},
                    {typed::CollabAgentToolCallStatus::failed(), "failed"},
                },
                knownValueCount) &&
            exactKnownValues<typed::CommandExecutionSource>(
                {
                    {typed::CommandExecutionSource::agent(), "agent"},
                    {typed::CommandExecutionSource::userShell(), "userShell"},
                    {typed::CommandExecutionSource::unifiedExecStartup(), "unifiedExecStartup"},
                    {typed::CommandExecutionSource::unifiedExecInteraction(), "unifiedExecInteraction"},
                },
                knownValueCount) &&
            exactKnownValues<typed::CommandExecutionStatus>(
                {
                    {typed::CommandExecutionStatus::inProgress(), "inProgress"},
                    {typed::CommandExecutionStatus::completed(), "completed"},
                    {typed::CommandExecutionStatus::failed(), "failed"},
                    {typed::CommandExecutionStatus::declined(), "declined"},
                },
                knownValueCount) &&
            exactKnownValues<typed::DynamicToolCallStatus>(
                {
                    {typed::DynamicToolCallStatus::inProgress(), "inProgress"},
                    {typed::DynamicToolCallStatus::completed(), "completed"},
                    {typed::DynamicToolCallStatus::failed(), "failed"},
                },
                knownValueCount) &&
            exactKnownValues<typed::LocalShellStatus>(
                {
                    {typed::LocalShellStatus::completed(), "completed"},
                    {typed::LocalShellStatus::inProgress(), "in_progress"},
                    {typed::LocalShellStatus::incomplete(), "incomplete"},
                },
                knownValueCount) &&
            exactKnownValues<typed::McpToolCallStatus>(
                {
                    {typed::McpToolCallStatus::inProgress(), "inProgress"},
                    {typed::McpToolCallStatus::completed(), "completed"},
                    {typed::McpToolCallStatus::failed(), "failed"},
                },
                knownValueCount) &&
            exactKnownValues<typed::PatchApplyStatus>(
                {
                    {typed::PatchApplyStatus::inProgress(), "inProgress"},
                    {typed::PatchApplyStatus::completed(), "completed"},
                    {typed::PatchApplyStatus::failed(), "failed"},
                    {typed::PatchApplyStatus::declined(), "declined"},
                },
                knownValueCount) &&
            exactKnownValues<typed::SubAgentActivityKind>(
                {
                    {typed::SubAgentActivityKind::started(), "started"},
                    {typed::SubAgentActivityKind::interacted(), "interacted"},
                    {typed::SubAgentActivityKind::interrupted(), "interrupted"},
                },
                knownValueCount);
        result.expectTrue(exact && knownValueCount == 41,
                          "all 39 B3 definition factories and two item-local phase factories expose exact pinned values and reject "
                          "future or empty values as known");
    }

} // namespace

int main() {
    tests::support::TestResult result;

    static_assert(std::variant_size_v<typed::ThreadItem> == 19);
    static_assert(std::variant_size_v<typed::ResponseItem> == 17);
    static_assert(!std::is_same_v<typed::ThreadItem, typed::ResponseItem>);
    static_assert(!std::is_same_v<typed::UnknownItem, typed::UnknownResponseItem>);
    static_assert(std::is_same_v<decltype(typed::CommandExecutionThreadItem::exitCode),
                                 typed::OptionalNullable<std::int32_t>>);
    static_assert(std::is_same_v<decltype(typed::CommandExecutionThreadItem::durationMs),
                                 typed::OptionalNullable<std::int64_t>>);
    static_assert(std::is_same_v<decltype(typed::MemoryCitationEntry::lineStart), std::uint32_t>);
    static_assert(std::is_same_v<decltype(typed::MemoryCitationEntry::lineEnd), std::uint32_t>);

    testRegistryAndDescriptors(result);
    testIndexedCorpus(result);
    testFutureOuterAlternatives(result);
    testSensitiveDiagnosticOmission(result);
    testIntegerBoundsAndLegacyReasoningOrder(result);
    testB3OpenEnumFactories(result);

    return result.processResult();
}
