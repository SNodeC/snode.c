/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_ITEMS_H
#define AI_OPENAI_CODEX_TYPED_ITEMS_H

#include "ai/openai/codex/typed/Conversation.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct MessagePhase {
        std::string value;

        [[nodiscard]] static MessagePhase commentary() {
            return {"commentary"};
        }

        [[nodiscard]] static MessagePhase finalAnswer() {
            return {"final_answer"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "commentary" || value == "final_answer";
        }

        auto operator<=>(const MessagePhase&) const = default;
    };

    struct CollabAgentStatus {
        std::string value;

        [[nodiscard]] static CollabAgentStatus pendingInit() {
            return {"pendingInit"};
        }

        [[nodiscard]] static CollabAgentStatus running() {
            return {"running"};
        }

        [[nodiscard]] static CollabAgentStatus interrupted() {
            return {"interrupted"};
        }

        [[nodiscard]] static CollabAgentStatus completed() {
            return {"completed"};
        }

        [[nodiscard]] static CollabAgentStatus errored() {
            return {"errored"};
        }

        [[nodiscard]] static CollabAgentStatus shutdown() {
            return {"shutdown"};
        }

        [[nodiscard]] static CollabAgentStatus notFound() {
            return {"notFound"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "pendingInit" || value == "running" || value == "interrupted" || value == "completed" || value == "errored" ||
                   value == "shutdown" || value == "notFound";
        }

        auto operator<=>(const CollabAgentStatus&) const = default;
    };

    struct CollabAgentTool {
        std::string value;

        [[nodiscard]] static CollabAgentTool spawnAgent() {
            return {"spawnAgent"};
        }

        [[nodiscard]] static CollabAgentTool sendInput() {
            return {"sendInput"};
        }

        [[nodiscard]] static CollabAgentTool resumeAgent() {
            return {"resumeAgent"};
        }

        [[nodiscard]] static CollabAgentTool wait() {
            return {"wait"};
        }

        [[nodiscard]] static CollabAgentTool closeAgent() {
            return {"closeAgent"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "spawnAgent" || value == "sendInput" || value == "resumeAgent" || value == "wait" || value == "closeAgent";
        }

        auto operator<=>(const CollabAgentTool&) const = default;
    };

    struct CollabAgentToolCallStatus {
        std::string value;

        [[nodiscard]] static CollabAgentToolCallStatus inProgress() {
            return {"inProgress"};
        }

        [[nodiscard]] static CollabAgentToolCallStatus completed() {
            return {"completed"};
        }

        [[nodiscard]] static CollabAgentToolCallStatus failed() {
            return {"failed"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "inProgress" || value == "completed" || value == "failed";
        }

        auto operator<=>(const CollabAgentToolCallStatus&) const = default;
    };

    struct CommandExecutionSource {
        std::string value;

        [[nodiscard]] static CommandExecutionSource agent() {
            return {"agent"};
        }

        [[nodiscard]] static CommandExecutionSource userShell() {
            return {"userShell"};
        }

        [[nodiscard]] static CommandExecutionSource unifiedExecStartup() {
            return {"unifiedExecStartup"};
        }

        [[nodiscard]] static CommandExecutionSource unifiedExecInteraction() {
            return {"unifiedExecInteraction"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "agent" || value == "userShell" || value == "unifiedExecStartup" || value == "unifiedExecInteraction";
        }

        auto operator<=>(const CommandExecutionSource&) const = default;
    };

    struct CommandExecutionStatus {
        std::string value;

        [[nodiscard]] static CommandExecutionStatus inProgress() {
            return {"inProgress"};
        }

        [[nodiscard]] static CommandExecutionStatus completed() {
            return {"completed"};
        }

        [[nodiscard]] static CommandExecutionStatus failed() {
            return {"failed"};
        }

        [[nodiscard]] static CommandExecutionStatus declined() {
            return {"declined"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "inProgress" || value == "completed" || value == "failed" || value == "declined";
        }

        auto operator<=>(const CommandExecutionStatus&) const = default;
    };

    struct DynamicToolCallStatus {
        std::string value;

        [[nodiscard]] static DynamicToolCallStatus inProgress() {
            return {"inProgress"};
        }

        [[nodiscard]] static DynamicToolCallStatus completed() {
            return {"completed"};
        }

        [[nodiscard]] static DynamicToolCallStatus failed() {
            return {"failed"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "inProgress" || value == "completed" || value == "failed";
        }

        auto operator<=>(const DynamicToolCallStatus&) const = default;
    };

    struct LocalShellStatus {
        std::string value;

        [[nodiscard]] static LocalShellStatus completed() {
            return {"completed"};
        }

        [[nodiscard]] static LocalShellStatus inProgress() {
            return {"in_progress"};
        }

        [[nodiscard]] static LocalShellStatus incomplete() {
            return {"incomplete"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "completed" || value == "in_progress" || value == "incomplete";
        }

        auto operator<=>(const LocalShellStatus&) const = default;
    };

    struct McpToolCallStatus {
        std::string value;

        [[nodiscard]] static McpToolCallStatus inProgress() {
            return {"inProgress"};
        }

        [[nodiscard]] static McpToolCallStatus completed() {
            return {"completed"};
        }

        [[nodiscard]] static McpToolCallStatus failed() {
            return {"failed"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "inProgress" || value == "completed" || value == "failed";
        }

        auto operator<=>(const McpToolCallStatus&) const = default;
    };

    struct PatchApplyStatus {
        std::string value;

        [[nodiscard]] static PatchApplyStatus inProgress() {
            return {"inProgress"};
        }

        [[nodiscard]] static PatchApplyStatus completed() {
            return {"completed"};
        }

        [[nodiscard]] static PatchApplyStatus failed() {
            return {"failed"};
        }

        [[nodiscard]] static PatchApplyStatus declined() {
            return {"declined"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "inProgress" || value == "completed" || value == "failed" || value == "declined";
        }

        auto operator<=>(const PatchApplyStatus&) const = default;
    };

    struct SubAgentActivityKind {
        std::string value;

        [[nodiscard]] static SubAgentActivityKind started() {
            return {"started"};
        }

        [[nodiscard]] static SubAgentActivityKind interacted() {
            return {"interacted"};
        }

        [[nodiscard]] static SubAgentActivityKind interrupted() {
            return {"interrupted"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "started" || value == "interacted" || value == "interrupted";
        }

        auto operator<=>(const SubAgentActivityKind&) const = default;
    };

    struct LegacyAppPathString {
        std::string value;

        LegacyAppPathString() = default;

        // The stable wire type is a direction-specific path string. Keep the
        // former string-based item construction source-compatible where the
        // conversion is unambiguous.
        LegacyAppPathString(std::string input)
            : value(std::move(input)) {
        }

        auto operator<=>(const LegacyAppPathString&) const = default;
    };

    struct CollabAgentState {
        CollabAgentStatus status;
        OptionalNullable<std::string> message;

        bool operator==(const CollabAgentState&) const = default;
    };

    struct FileUpdateChange {
        std::string diff;
        PatchChangeKind kind;
        std::string path;

        bool operator==(const FileUpdateChange&) const = default;
    };

    struct HookPromptFragment {
        std::string hookRunId;
        std::string text;

        bool operator==(const HookPromptFragment&) const = default;
    };

    struct InternalChatMessageMetadataPassthrough {
        OptionalNullable<TurnId> turnId;

        bool operator==(const InternalChatMessageMetadataPassthrough&) const = default;
    };

    struct McpToolCallAppContext {
        std::string connectorId;
        OptionalNullable<std::string> actionName;
        OptionalNullable<std::string> appName;
        OptionalNullable<std::string> linkId;
        OptionalNullable<std::string> resourceUri;
        OptionalNullable<std::string> templateId;

        bool operator==(const McpToolCallAppContext&) const = default;
    };

    struct McpToolCallError {
        std::string message;

        bool operator==(const McpToolCallError&) const = default;
    };

    struct McpToolCallResult {
        std::vector<Json> content;
        OptionalNullable<Json> meta;
        OptionalNullable<Json> structuredContent;

        bool operator==(const McpToolCallResult&) const = default;
    };

    struct MemoryCitationEntry {
        std::uint32_t lineEnd = 0;
        std::uint32_t lineStart = 0;
        std::string note;
        std::string path;

        bool operator==(const MemoryCitationEntry&) const = default;
    };

    struct MemoryCitation {
        std::vector<MemoryCitationEntry> entries;
        std::vector<ThreadId> threadIds;

        bool operator==(const MemoryCitation&) const = default;
    };

    struct InputTextAgentMessageInputContent {
        std::string text;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const InputTextAgentMessageInputContent&) const = default;
    };

    struct EncryptedContentAgentMessageInputContent {
        std::string encryptedContent;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const EncryptedContentAgentMessageInputContent&) const = default;
    };

    struct UnknownAgentMessageInputContent {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownAgentMessageInputContent&) const = default;
    };

    using AgentMessageInputContent =
        std::variant<InputTextAgentMessageInputContent, EncryptedContentAgentMessageInputContent, UnknownAgentMessageInputContent>;

    struct InputTextContentItem {
        std::string text;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const InputTextContentItem&) const = default;
    };

    struct InputImageContentItem {
        std::string imageUrl;
        OptionalNullable<ImageDetail> detail;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const InputImageContentItem&) const = default;
    };

    struct OutputTextContentItem {
        std::string text;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const OutputTextContentItem&) const = default;
    };

    struct UnknownContentItem {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownContentItem&) const = default;
    };

    using ContentItem = std::variant<InputTextContentItem, InputImageContentItem, OutputTextContentItem, UnknownContentItem>;

    struct InputTextFunctionCallOutputContentItem {
        std::string text;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const InputTextFunctionCallOutputContentItem&) const = default;
    };

    struct InputImageFunctionCallOutputContentItem {
        std::string imageUrl;
        OptionalNullable<ImageDetail> detail;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const InputImageFunctionCallOutputContentItem&) const = default;
    };

    struct EncryptedContentFunctionCallOutputContentItem {
        std::string encryptedContent;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const EncryptedContentFunctionCallOutputContentItem&) const = default;
    };

    struct UnknownFunctionCallOutputContentItem {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownFunctionCallOutputContentItem&) const = default;
    };

    using FunctionCallOutputContentItem = std::variant<InputTextFunctionCallOutputContentItem,
                                                       InputImageFunctionCallOutputContentItem,
                                                       EncryptedContentFunctionCallOutputContentItem,
                                                       UnknownFunctionCallOutputContentItem>;

    using FunctionCallOutputBody = std::variant<std::string, std::vector<FunctionCallOutputContentItem>>;

    struct ExecLocalShellAction {
        std::vector<std::string> command;
        OptionalNullable<std::map<std::string, std::string>> env;
        OptionalNullable<std::uint64_t> timeoutMs;
        OptionalNullable<std::string> user;
        OptionalNullable<std::string> workingDirectory;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ExecLocalShellAction&) const = default;
    };

    struct UnknownLocalShellAction {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownLocalShellAction&) const = default;
    };

    using LocalShellAction = std::variant<ExecLocalShellAction, UnknownLocalShellAction>;

    struct ReasoningTextReasoningItemContent {
        std::string text;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ReasoningTextReasoningItemContent&) const = default;
    };

    struct TextReasoningItemContent {
        std::string text;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const TextReasoningItemContent&) const = default;
    };

    struct UnknownReasoningItemContent {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownReasoningItemContent&) const = default;
    };

    using ReasoningItemContent = std::variant<ReasoningTextReasoningItemContent, TextReasoningItemContent, UnknownReasoningItemContent>;

    struct SummaryTextReasoningItemReasoningSummary {
        std::string text;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SummaryTextReasoningItemReasoningSummary&) const = default;
    };

    struct UnknownReasoningItemReasoningSummary {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownReasoningItemReasoningSummary&) const = default;
    };

    using ReasoningItemReasoningSummary = std::variant<SummaryTextReasoningItemReasoningSummary, UnknownReasoningItemReasoningSummary>;

    struct SearchResponsesApiWebSearchAction {
        OptionalNullable<std::vector<std::string>> queries;
        OptionalNullable<std::string> query;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SearchResponsesApiWebSearchAction&) const = default;
    };

    struct OpenPageResponsesApiWebSearchAction {
        OptionalNullable<std::string> url;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const OpenPageResponsesApiWebSearchAction&) const = default;
    };

    struct FindInPageResponsesApiWebSearchAction {
        OptionalNullable<std::string> pattern;
        OptionalNullable<std::string> url;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FindInPageResponsesApiWebSearchAction&) const = default;
    };

    struct OtherResponsesApiWebSearchAction {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const OtherResponsesApiWebSearchAction&) const = default;
    };

    struct UnknownResponsesApiWebSearchAction {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownResponsesApiWebSearchAction&) const = default;
    };

    using ResponsesApiWebSearchAction = std::variant<SearchResponsesApiWebSearchAction,
                                                     OpenPageResponsesApiWebSearchAction,
                                                     FindInPageResponsesApiWebSearchAction,
                                                     OtherResponsesApiWebSearchAction,
                                                     UnknownResponsesApiWebSearchAction>;

    struct ItemMetadata {
        ItemId id;
        std::optional<ThreadId> threadId;
        std::optional<TurnId> turnId;
        Json raw = Json::object();

        bool operator==(const ItemMetadata&) const = default;
    };

    struct AgentMessageThreadItem {
        ItemMetadata metadata;
        std::string text;
        OptionalNullable<MemoryCitation> memoryCitation;
        OptionalNullable<MessagePhase> phase;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AgentMessageThreadItem&) const = default;
    };

    struct CollabAgentToolCallThreadItem {
        ItemMetadata metadata;
        std::map<std::string, CollabAgentState> agentsStates;
        std::vector<ThreadId> receiverThreadIds;
        ThreadId senderThreadId;
        CollabAgentToolCallStatus status;
        CollabAgentTool tool;
        OptionalNullable<ModelId> model;
        OptionalNullable<std::string> prompt;
        OptionalNullable<ReasoningEffort> reasoningEffort;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const CollabAgentToolCallThreadItem&) const = default;
    };

    struct CommandExecutionThreadItem {
        ItemMetadata metadata;
        std::string command;
        std::vector<CommandAction> commandActions;
        LegacyAppPathString cwd;
        CommandExecutionStatus status;
        OptionalNullable<std::string> aggregatedOutput;
        OptionalNullable<std::int64_t> durationMs;
        OptionalNullable<std::int32_t> exitCode;
        OptionalNullable<std::string> processId;
        std::optional<CommandExecutionSource> source;
        std::vector<DecodeDiagnostic> diagnostics;

        [[nodiscard]] CommandExecutionSource sourceOrDefault() const {
            return source.value_or(CommandExecutionSource::agent());
        }

        bool operator==(const CommandExecutionThreadItem&) const = default;
    };

    struct ContextCompactionThreadItem {
        ItemMetadata metadata;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ContextCompactionThreadItem&) const = default;
    };

    struct DynamicToolCallThreadItem {
        ItemMetadata metadata;
        std::optional<Json> arguments;
        DynamicToolCallStatus status;
        std::string tool;
        OptionalNullable<std::vector<DynamicToolCallOutputContentItem>> contentItems;
        OptionalNullable<std::int64_t> durationMs;
        OptionalNullable<std::string> nameSpace;
        OptionalNullable<bool> success;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const DynamicToolCallThreadItem&) const = default;
    };

    struct EnteredReviewModeThreadItem {
        ItemMetadata metadata;
        std::string review;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const EnteredReviewModeThreadItem&) const = default;
    };

    struct ExitedReviewModeThreadItem {
        ItemMetadata metadata;
        std::string review;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ExitedReviewModeThreadItem&) const = default;
    };

    struct FileChangeThreadItem {
        ItemMetadata metadata;
        std::vector<FileUpdateChange> changes;
        PatchApplyStatus status;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FileChangeThreadItem&) const = default;
    };

    struct HookPromptThreadItem {
        ItemMetadata metadata;
        std::vector<HookPromptFragment> fragments;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const HookPromptThreadItem&) const = default;
    };

    struct ImageGenerationThreadItem {
        ItemMetadata metadata;
        std::string result;
        std::string status;
        OptionalNullable<std::string> revisedPrompt;
        OptionalNullable<AbsolutePathBuf> savedPath;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ImageGenerationThreadItem&) const = default;
    };

    struct ImageViewThreadItem {
        ItemMetadata metadata;
        LegacyAppPathString path;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ImageViewThreadItem&) const = default;
    };

    struct McpToolCallThreadItem {
        ItemMetadata metadata;
        std::optional<Json> arguments;
        std::string server;
        McpToolCallStatus status;
        std::string tool;
        OptionalNullable<McpToolCallAppContext> appContext;
        OptionalNullable<std::int64_t> durationMs;
        OptionalNullable<McpToolCallError> error;
        OptionalNullable<std::string> mcpAppResourceUri;
        OptionalNullable<std::string> pluginId;
        OptionalNullable<McpToolCallResult> result;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const McpToolCallThreadItem&) const = default;
    };

    struct PlanThreadItem {
        ItemMetadata metadata;
        std::string text;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const PlanThreadItem&) const = default;
    };

    struct ReasoningThreadItem {
        ItemMetadata metadata;
        std::optional<std::vector<std::string>> summary;
        std::optional<std::vector<std::string>> content;
        std::vector<DecodeDiagnostic> diagnostics;

        [[nodiscard]] const std::vector<std::string>& contentOrDefault() const {
            static const std::vector<std::string> empty;
            return content ? *content : empty;
        }

        [[nodiscard]] const std::vector<std::string>& summaryOrDefault() const {
            static const std::vector<std::string> empty;
            return summary ? *summary : empty;
        }

        bool operator==(const ReasoningThreadItem&) const = default;
    };

    struct SleepThreadItem {
        ItemMetadata metadata;
        std::uint64_t durationMs = 0;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SleepThreadItem&) const = default;
    };

    struct SubAgentActivityThreadItem {
        ItemMetadata metadata;
        std::string agentPath;
        ThreadId agentThreadId;
        SubAgentActivityKind kind;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SubAgentActivityThreadItem&) const = default;
    };

    struct UserMessageThreadItem {
        ItemMetadata metadata;
        std::vector<UserInput> content;
        OptionalNullable<ClientUserMessageId> clientId;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const UserMessageThreadItem&) const = default;
    };

    struct WebSearchThreadItem {
        ItemMetadata metadata;
        std::string query;
        OptionalNullable<WebSearchAction> action;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const WebSearchThreadItem&) const = default;
    };

    struct UnknownItemMetadata {
        std::optional<ItemId> id;
        std::optional<ThreadId> threadId;
        std::optional<TurnId> turnId;

        bool operator==(const UnknownItemMetadata&) const = default;
    };

    struct UnknownItem {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<std::string> decodingError;
        UnknownItemMetadata metadata = {};
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownItem&) const = default;
    };

    using ThreadItem = std::variant<AgentMessageThreadItem,
                                    CollabAgentToolCallThreadItem,
                                    CommandExecutionThreadItem,
                                    ContextCompactionThreadItem,
                                    DynamicToolCallThreadItem,
                                    EnteredReviewModeThreadItem,
                                    ExitedReviewModeThreadItem,
                                    FileChangeThreadItem,
                                    HookPromptThreadItem,
                                    ImageGenerationThreadItem,
                                    ImageViewThreadItem,
                                    McpToolCallThreadItem,
                                    PlanThreadItem,
                                    ReasoningThreadItem,
                                    SleepThreadItem,
                                    SubAgentActivityThreadItem,
                                    UserMessageThreadItem,
                                    WebSearchThreadItem,
                                    UnknownItem>;

    // Transitional source compatibility for the A0/A1.0 item vocabulary.
    using AgentMessageItem = AgentMessageThreadItem;
    using CommandExecutionItem = CommandExecutionThreadItem;
    using FileChangeItem = FileChangeThreadItem;
    using ToolCallItem = McpToolCallThreadItem;
    using ReasoningItem = ReasoningThreadItem;
    using UserMessageItem = UserMessageThreadItem;
    using WebSearchItem = WebSearchThreadItem;
    using Item = ThreadItem;

    struct MessageResponseItem {
        std::vector<ContentItem> content;
        std::string role;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        OptionalNullable<MessagePhase> phase;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const MessageResponseItem&) const = default;
    };

    struct AgentMessageResponseItem {
        std::string author;
        std::vector<AgentMessageInputContent> content;
        std::string recipient;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AgentMessageResponseItem&) const = default;
    };

    struct ReasoningResponseItem {
        std::vector<ReasoningItemReasoningSummary> summary;
        OptionalNullable<std::vector<ReasoningItemContent>> content;
        OptionalNullable<std::string> encryptedContent;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ReasoningResponseItem&) const = default;
    };

    struct LocalShellCallResponseItem {
        LocalShellAction action;
        LocalShellStatus status;
        OptionalNullable<ResponseCallId> callId;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const LocalShellCallResponseItem&) const = default;
    };

    struct FunctionCallResponseItem {
        std::string arguments;
        ResponseCallId callId;
        std::string name;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        OptionalNullable<std::string> nameSpace;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FunctionCallResponseItem&) const = default;
    };

    struct ToolSearchCallResponseItem {
        std::optional<Json> arguments;
        std::string execution;
        OptionalNullable<ResponseCallId> callId;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        OptionalNullable<std::string> status;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ToolSearchCallResponseItem&) const = default;
    };

    struct FunctionCallOutputResponseItem {
        ResponseCallId callId;
        FunctionCallOutputBody output;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FunctionCallOutputResponseItem&) const = default;
    };

    struct CustomToolCallResponseItem {
        ResponseCallId callId;
        std::string input;
        std::string name;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        OptionalNullable<std::string> nameSpace;
        OptionalNullable<std::string> status;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const CustomToolCallResponseItem&) const = default;
    };

    struct CustomToolCallOutputResponseItem {
        ResponseCallId callId;
        FunctionCallOutputBody output;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        OptionalNullable<std::string> name;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const CustomToolCallOutputResponseItem&) const = default;
    };

    struct ToolSearchOutputResponseItem {
        std::string execution;
        std::string status;
        std::vector<Json> tools;
        OptionalNullable<ResponseCallId> callId;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ToolSearchOutputResponseItem&) const = default;
    };

    struct WebSearchCallResponseItem {
        OptionalNullable<ResponsesApiWebSearchAction> action;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        OptionalNullable<std::string> status;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const WebSearchCallResponseItem&) const = default;
    };

    struct ImageGenerationCallResponseItem {
        std::string result;
        std::string status;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        OptionalNullable<std::string> revisedPrompt;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ImageGenerationCallResponseItem&) const = default;
    };

    struct CompactionResponseItem {
        std::string encryptedContent;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const CompactionResponseItem&) const = default;
    };

    struct CompactionTriggerResponseItem {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const CompactionTriggerResponseItem&) const = default;
    };

    struct ContextCompactionResponseItem {
        OptionalNullable<std::string> encryptedContent;
        OptionalNullable<ResponseItemId> id;
        OptionalNullable<InternalChatMessageMetadataPassthrough> internalChatMessageMetadataPassthrough;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ContextCompactionResponseItem&) const = default;
    };

    struct OtherResponseItem {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const OtherResponseItem&) const = default;
    };

    struct UnknownResponseItemMetadata {
        std::optional<ResponseItemId> id;

        bool operator==(const UnknownResponseItemMetadata&) const = default;
    };

    struct UnknownResponseItem {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<std::string> decodingError;
        UnknownResponseItemMetadata metadata = {};
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownResponseItem&) const = default;
    };

    using ResponseItem = std::variant<AgentMessageResponseItem,
                                      CompactionResponseItem,
                                      CompactionTriggerResponseItem,
                                      ContextCompactionResponseItem,
                                      CustomToolCallResponseItem,
                                      CustomToolCallOutputResponseItem,
                                      FunctionCallResponseItem,
                                      FunctionCallOutputResponseItem,
                                      ImageGenerationCallResponseItem,
                                      LocalShellCallResponseItem,
                                      MessageResponseItem,
                                      OtherResponseItem,
                                      ReasoningResponseItem,
                                      ToolSearchCallResponseItem,
                                      ToolSearchOutputResponseItem,
                                      WebSearchCallResponseItem,
                                      UnknownResponseItem>;

    static_assert(!std::is_same_v<ThreadItem, ResponseItem>);

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_ITEMS_H
