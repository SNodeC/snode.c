/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/BackendAdapter.h"

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/backend/BackendCommand.h"
#include "ai/openai/codex/backend/BackendCore.h"
#include "ai/openai/codex/backend/BackendEvent.h"
#include "ai/openai/codex/backend/BackendState.h"
#include "ai/openai/codex/backend/FrontendSession.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "ai/openai/codex/frontend/Protocol.h"
#include "ai/openai/codex/typed/ServerRequests.h"
#include "ai/openai/codex/typed/Threads.h"
#include "ai/openai/codex/typed/Turns.h"
#include "ai/openai/codex/typed/Types.h"
#include "core/EventReceiver.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <deque>
#include <exception>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <system_error>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::frontend {

    namespace {

        template <typename... Visitors>
        struct Overloaded : Visitors... {
            using Visitors::operator()...;
        };

        template <typename... Visitors>
        Overloaded(Visitors...) -> Overloaded<Visitors...>;

        std::string backendLifecycleName(backend::BackendLifecycle lifecycle) {
            switch (lifecycle) {
                case backend::BackendLifecycle::Stopped:
                    return "stopped";
                case backend::BackendLifecycle::Starting:
                    return "starting";
                case backend::BackendLifecycle::Initializing:
                    return "initializing";
                case backend::BackendLifecycle::Ready:
                    return "ready";
                case backend::BackendLifecycle::Stopping:
                    return "stopping";
                case backend::BackendLifecycle::Failed:
                    return "failed";
            }
            return "failed";
        }

        std::string backendRoleName(backend::SessionRole role) {
            return role == backend::SessionRole::Controller ? "controller" : "observer";
        }

        Json errorSnapshotJson(const backend::ErrorSnapshot& error) {
            return Json{{"category", error.category}, {"code", error.code}, {"message", error.message}};
        }

        Json itemSnapshotJson(const backend::ItemSnapshot& item) {
            Json encoded{{"id", item.id},
                         {"type", item.type},
                         {"status", item.status},
                         {"agentText", item.agentText},
                         {"reasoningText", item.reasoningText},
                         {"reasoningSummary", item.reasoningSummary},
                         {"commandOutput", item.commandOutput},
                         {"droppedContentBytes", item.droppedContentBytes},
                         {"contentTruncated", item.contentTruncated},
                         {"data", item.data},
                         {"extensions", item.extensions}};
            if (item.startedAtMs.has_value()) {
                encoded["startedAtMs"] = *item.startedAtMs;
            }
            if (item.completedAtMs.has_value()) {
                encoded["completedAtMs"] = *item.completedAtMs;
            }
            return encoded;
        }

        Json turnSnapshotJson(const backend::TurnSnapshot& turn) {
            Json encoded{{"id", turn.id},
                         {"threadId", turn.threadId},
                         {"status", turn.status},
                         {"active", turn.active},
                         {"terminal", turn.terminal},
                         {"items", Json::array()},
                         {"extensions", turn.extensions}};
            if (turn.failure.has_value()) {
                encoded["failure"] = *turn.failure;
            }
            if (turn.tokenUsage.has_value()) {
                encoded["tokenUsage"] = *turn.tokenUsage;
            }
            for (const backend::ItemSnapshot& item : turn.items) {
                encoded["items"].push_back(itemSnapshotJson(item));
            }
            return encoded;
        }

        Json threadSnapshotJson(const backend::ThreadSnapshot& thread) {
            Json encoded{
                {"id", thread.id}, {"fullyLoaded", thread.fullyLoaded}, {"turns", Json::array()}, {"extensions", thread.extensions}};
            if (thread.title.has_value()) {
                encoded["title"] = *thread.title;
            }
            if (thread.cwd.has_value()) {
                encoded["cwd"] = *thread.cwd;
            }
            if (thread.model.has_value()) {
                encoded["model"] = *thread.model;
            }
            if (thread.modelProvider.has_value()) {
                encoded["modelProvider"] = *thread.modelProvider;
            }
            if (thread.preview.has_value()) {
                encoded["preview"] = *thread.preview;
            }
            if (thread.status.has_value()) {
                encoded["status"] = *thread.status;
            }
            if (thread.createdAt.has_value()) {
                encoded["createdAt"] = *thread.createdAt;
            }
            if (thread.updatedAt.has_value()) {
                encoded["updatedAt"] = *thread.updatedAt;
            }
            for (const backend::TurnSnapshot& turn : thread.turns) {
                encoded["turns"].push_back(turnSnapshotJson(turn));
            }
            return encoded;
        }

        Json pendingRequestSnapshotJson(const backend::PendingRequestSnapshot& pending) {
            Json encoded{{"id", std::to_string(pending.id.value())}, {"type", pending.type}, {"details", pending.details}};
            if (pending.threadId.has_value()) {
                encoded["threadId"] = *pending.threadId;
            }
            if (pending.turnId.has_value()) {
                encoded["turnId"] = *pending.turnId;
            }
            if (pending.itemId.has_value()) {
                encoded["itemId"] = *pending.itemId;
            }
            return encoded;
        }

        Json extensionSnapshotJson(const backend::ExtensionSnapshot& extension) {
            Json encoded{{"method", extension.method}, {"params", extension.payload}};
            if (extension.decodingError.has_value()) {
                encoded["decodingError"] = *extension.decodingError;
            }
            if (extension.sensitiveFieldsRedacted) {
                encoded["sensitiveFieldsRedacted"] = true;
            }
            Json truncation = Json::object();
            if (extension.methodTruncated) {
                truncation["method"] = {{"originalBytes", extension.originalMethodBytes}, {"retainedBytes", extension.method.size()}};
            }
            if (extension.payloadTruncated) {
                truncation["params"] = Json::object();
                if (extension.originalPayloadBytes.has_value()) {
                    truncation["params"]["originalBytes"] = *extension.originalPayloadBytes;
                }
            }
            if (extension.decodingErrorTruncated) {
                truncation["decodingError"] = {{"originalBytes", extension.originalDecodingErrorBytes},
                                               {"retainedBytes", extension.decodingError ? extension.decodingError->size() : 0}};
            }
            if (!truncation.empty()) {
                encoded["truncation"] = std::move(truncation);
            }
            return encoded;
        }

        const backend::ThreadSnapshot* findThread(const backend::Snapshot& snapshot, std::string_view threadId) {
            for (const backend::ThreadSnapshot& thread : snapshot.threads) {
                if (thread.id == threadId) {
                    return &thread;
                }
            }
            return nullptr;
        }

        const backend::TurnSnapshot* findTurn(const backend::Snapshot& snapshot, std::string_view threadId, std::string_view turnId) {
            const backend::ThreadSnapshot* thread = findThread(snapshot, threadId);
            if (thread == nullptr) {
                return nullptr;
            }
            for (const backend::TurnSnapshot& turn : thread->turns) {
                if (turn.id == turnId) {
                    return &turn;
                }
            }
            return nullptr;
        }

        const backend::ItemSnapshot*
        findItem(const backend::Snapshot& snapshot, std::string_view threadId, std::string_view turnId, std::string_view itemId) {
            const backend::TurnSnapshot* turn = findTurn(snapshot, threadId, turnId);
            if (turn == nullptr) {
                return nullptr;
            }
            for (const backend::ItemSnapshot& item : turn->items) {
                if (item.id == itemId) {
                    return &item;
                }
            }
            return nullptr;
        }

        const backend::PendingRequestSnapshot* findPending(const backend::Snapshot& snapshot, backend::PendingRequestId id) {
            for (const backend::PendingRequestSnapshot& pending : snapshot.pendingRequests) {
                if (pending.id == id) {
                    return &pending;
                }
            }
            return nullptr;
        }

        Json backendSnapshotJson(const backend::Snapshot& snapshot,
                                 SequenceNumber frontendSequence,
                                 SequenceNumber oldestReplayableAfter,
                                 std::optional<SequenceNumber> oldestRetained,
                                 std::optional<SequenceNumber> newestRetained) {
            Json encoded{
                {"backendRevision", snapshot.sequence.value()},
                {"lifecycle", backendLifecycleName(snapshot.lifecycle)},
                {"diagnostics", {{"received", snapshot.diagnostics.received}, {"recent", snapshot.diagnostics.recent}}},
                {"threads", Json::array()},
                {"pendingRequests", Json::array()},
                {"sessions", Json::array()},
                {"codexExtensions", Json::array()},
                {"omittedCodexExtensions", snapshot.omittedRecentExtensions},
                {"threadList",
                 {{"hasLoadedPage", snapshot.threadList.hasLoadedPage},
                  {"complete", snapshot.threadList.complete},
                  {"pagesLoaded", snapshot.threadList.pagesLoaded}}},
                {"journal", {{"oldestReplayableAfter", oldestReplayableAfter.value()}, {"currentSequence", frontendSequence.value()}}},
                {"sequenceExhausted", snapshot.sequenceExhausted}};
            if (snapshot.lastLifecycleError.has_value()) {
                encoded["lastLifecycleError"] = errorSnapshotJson(*snapshot.lastLifecycleError);
            }
            for (const backend::ThreadSnapshot& thread : snapshot.threads) {
                encoded["threads"].push_back(threadSnapshotJson(thread));
            }
            for (const backend::PendingRequestSnapshot& pending : snapshot.pendingRequests) {
                encoded["pendingRequests"].push_back(pendingRequestSnapshotJson(pending));
            }
            if (snapshot.controller.has_value()) {
                encoded["controllerSessionId"] = std::to_string(snapshot.controller->value());
            }
            for (const backend::SessionSnapshot& session : snapshot.sessions) {
                encoded["sessions"].push_back({{"sessionId", std::to_string(session.id.value())}, {"role", backendRoleName(session.role)}});
            }
            for (const backend::ExtensionSnapshot& extension : snapshot.recentExtensions) {
                encoded["codexExtensions"].push_back(extensionSnapshotJson(extension));
            }
            if (snapshot.threadList.nextCursor.has_value()) {
                encoded["threadList"]["nextCursor"] = *snapshot.threadList.nextCursor;
            }
            if (snapshot.threadList.backwardsCursor.has_value()) {
                encoded["threadList"]["backwardsCursor"] = *snapshot.threadList.backwardsCursor;
            }
            if (oldestRetained.has_value()) {
                encoded["journal"]["oldestRetainedSequence"] = oldestRetained->value();
            }
            if (newestRetained.has_value()) {
                encoded["journal"]["newestRetainedSequence"] = newestRetained->value();
            }
            return encoded;
        }

        ErrorCode frontendErrorCode(backend::CommandErrorCode code) {
            switch (code) {
                case backend::CommandErrorCode::PermissionDenied:
                    return ErrorCode::PermissionDenied;
                case backend::CommandErrorCode::InvalidCommand:
                    return ErrorCode::InvalidCommand;
                case backend::CommandErrorCode::NotFound:
                    return ErrorCode::NotFound;
                case backend::CommandErrorCode::Conflict:
                    return ErrorCode::Conflict;
                case backend::CommandErrorCode::LocalSubmissionFailure:
                    return ErrorCode::LocalSubmissionFailure;
                case backend::CommandErrorCode::TypedDecodingFailure:
                    return ErrorCode::TypedDecodingFailure;
                case backend::CommandErrorCode::RemoteAppServerError:
                    return ErrorCode::RemoteAppServerError;
                case backend::CommandErrorCode::Cancelled:
                    return ErrorCode::Cancelled;
                case backend::CommandErrorCode::BackendUnavailable:
                    return ErrorCode::BackendUnavailable;
            }
            return ErrorCode::InternalError;
        }

        std::optional<backend::PendingRequestId> parsePendingRequestId(std::string_view encoded) {
            std::uint64_t value = 0;
            const auto result = std::from_chars(encoded.data(), encoded.data() + encoded.size(), value);
            if (encoded.empty() || result.ec != std::errc() || result.ptr != encoded.data() + encoded.size() || value == 0) {
                return std::nullopt;
            }
            return backend::PendingRequestId(value);
        }

        typed::TurnInput typedTurnInput(const TurnInput& input) {
            return std::visit(
                []<typename Input>(const Input& value) -> typed::TurnInput {
                    using T = std::remove_cvref_t<Input>;
                    if constexpr (std::is_same_v<T, TextInput>) {
                        typed::TextInput result;
                        result.text = value.text;
                        return result;
                    } else if constexpr (std::is_same_v<T, ImageUrlInput>) {
                        typed::ImageUrlInput result;
                        result.url = value.url;
                        if (value.detail.has_value()) {
                            result.detail = typed::ImageDetail{*value.detail};
                        }
                        return result;
                    } else if constexpr (std::is_same_v<T, LocalImageInput>) {
                        typed::LocalImageInput result;
                        result.path = value.path;
                        if (value.detail.has_value()) {
                            result.detail = typed::ImageDetail{*value.detail};
                        }
                        return result;
                    } else if constexpr (std::is_same_v<T, SkillInput>) {
                        typed::SkillInput result;
                        result.name = value.name;
                        result.path = value.path;
                        return result;
                    } else {
                        typed::MentionInput result;
                        result.name = value.name;
                        result.path = value.path;
                        return result;
                    }
                },
                input);
        }

        typed::SandboxPolicy typedSandboxPolicy(const SandboxPolicy& policy) {
            if (policy.type == "dangerFullAccess") {
                return typed::DangerFullAccessSandboxPolicy{};
            }
            if (policy.type == "readOnly") {
                typed::ReadOnlySandboxPolicy result;
                if (policy.networkAccess.has_value()) {
                    result.networkAccess = std::get<bool>(*policy.networkAccess);
                }
                return result;
            }
            if (policy.type == "externalSandbox") {
                typed::ExternalSandboxPolicy result;
                if (policy.networkAccess.has_value()) {
                    result.networkAccess = typed::NetworkAccess{std::get<std::string>(*policy.networkAccess)};
                }
                return result;
            }
            typed::WorkspaceWriteSandboxPolicy result;
            if (!policy.writableRoots.empty()) {
                std::vector<typed::AbsolutePathBuf> roots;
                roots.reserve(policy.writableRoots.size());
                for (const std::string& root : policy.writableRoots) {
                    roots.emplace_back(root);
                }
                result.writableRoots = std::move(roots);
            }
            if (policy.networkAccess.has_value()) {
                result.networkAccess = std::get<bool>(*policy.networkAccess);
            }
            result.excludeTmpdirEnvVar = policy.excludeTmpdirEnvVar;
            result.excludeSlashTmp = policy.excludeSlashTmp;
            return result;
        }

        struct CommandMappingError {
            std::string message;
        };

        using BackendCommandMapping = std::variant<backend::BackendCommand, CommandMappingError>;

        BackendCommandMapping mapBackendCommand(const CommandParameters& parameters) {
            return std::visit(
                Overloaded{
                    [](const ControllerAcquire&) -> BackendCommandMapping {
                        return backend::ControllerAcquire{};
                    },
                    [](const ControllerRelease&) -> BackendCommandMapping {
                        return backend::ControllerRelease{};
                    },
                    [](const SnapshotGet&) -> BackendCommandMapping {
                        return backend::SnapshotGet{};
                    },
                    [](const ReplayAfter& value) -> BackendCommandMapping {
                        return backend::ReplayAfter{backend::SequenceNumber(value.after.value())};
                    },
                    [](const ThreadStart& value) -> BackendCommandMapping {
                        typed::ThreadStartOptions options;
                        options.cwd = value.cwd;
                        if (value.model.has_value()) {
                            options.model = typed::ModelId{*value.model};
                        }
                        options.modelProvider = value.modelProvider;
                        if (value.approvalPolicy.has_value()) {
                            options.approvalPolicy = typed::ApprovalPolicy{*value.approvalPolicy};
                        }
                        if (value.sandboxMode.has_value()) {
                            options.sandboxMode = typed::SandboxMode{*value.sandboxMode};
                        }
                        options.ephemeral = value.ephemeral;
                        return backend::ThreadStart{std::move(options)};
                    },
                    [](const ThreadResume& value) -> BackendCommandMapping {
                        typed::ThreadResumeOptions options;
                        options.cwd = value.cwd;
                        if (value.model.has_value()) {
                            options.model = typed::ModelId{*value.model};
                        }
                        options.modelProvider = value.modelProvider;
                        if (value.approvalPolicy.has_value()) {
                            options.approvalPolicy = typed::ApprovalPolicy{*value.approvalPolicy};
                        }
                        if (value.sandboxMode.has_value()) {
                            options.sandboxMode = typed::SandboxMode{*value.sandboxMode};
                        }
                        return backend::ThreadResume{typed::ThreadId{value.threadId}, std::move(options)};
                    },
                    [](const ThreadList& value) -> BackendCommandMapping {
                        return backend::ThreadList{typed::ThreadListOptions{value.cursor, value.limit, value.archived, value.searchTerm}};
                    },
                    [](const ThreadRead& value) -> BackendCommandMapping {
                        return backend::ThreadRead{typed::ThreadId{value.threadId}, typed::ThreadReadOptions{value.includeTurns}};
                    },
                    [](const TurnStart& value) -> BackendCommandMapping {
                        backend::TurnStart start;
                        start.threadId = typed::ThreadId{value.threadId};
                        start.input.reserve(value.input.size());
                        for (const TurnInput& input : value.input) {
                            start.input.push_back(typedTurnInput(input));
                        }
                        start.options.cwd = value.cwd;
                        if (value.model.has_value()) {
                            start.options.model = typed::ModelId{*value.model};
                        }
                        if (value.reasoningEffort.has_value()) {
                            start.options.reasoningEffort = typed::ReasoningEffort{*value.reasoningEffort};
                        }
                        if (value.approvalPolicy.has_value()) {
                            start.options.approvalPolicy = typed::ApprovalPolicy{*value.approvalPolicy};
                        }
                        if (value.sandboxPolicy.has_value()) {
                            start.options.sandboxPolicy = typedSandboxPolicy(*value.sandboxPolicy);
                        }
                        return start;
                    },
                    [](const TurnInterrupt& value) -> BackendCommandMapping {
                        return backend::TurnInterrupt{typed::ThreadId{value.threadId}, typed::TurnId{value.turnId}};
                    },
                    [](const ApprovalRespond& value) -> BackendCommandMapping {
                        const auto requestId = parsePendingRequestId(value.pendingRequestId);
                        if (!requestId.has_value()) {
                            return CommandMappingError{"pendingRequestId must be a non-zero unsigned decimal integer"};
                        }
                        return backend::ApprovalRespond{*requestId, typed::ApprovalDecision{value.decision}};
                    },
                    [](const UserInputRespond& value) -> BackendCommandMapping {
                        const auto requestId = parsePendingRequestId(value.pendingRequestId);
                        if (!requestId.has_value()) {
                            return CommandMappingError{"pendingRequestId must be a non-zero unsigned decimal integer"};
                        }
                        std::vector<typed::UserInputAnswer> answers;
                        answers.reserve(value.answers.size());
                        for (const UserInputAnswer& answer : value.answers) {
                            answers.push_back({answer.questionId, answer.answers});
                        }
                        return backend::UserInputRespond{*requestId, std::move(answers)};
                    },
                    [](const AuthenticationRespond& value) -> BackendCommandMapping {
                        const auto requestId = parsePendingRequestId(value.pendingRequestId);
                        if (!requestId.has_value()) {
                            return CommandMappingError{"pendingRequestId must be a non-zero unsigned decimal integer"};
                        }
                        return backend::AuthenticationRespond{
                            *requestId, typed::AuthenticationResponse{value.accessToken, value.chatgptAccountId, value.chatgptPlanType}};
                    },
                    [](const UnknownRequestRespond& value) -> BackendCommandMapping {
                        const auto requestId = parsePendingRequestId(value.pendingRequestId);
                        if (!requestId.has_value()) {
                            return CommandMappingError{"pendingRequestId must be a non-zero unsigned decimal integer"};
                        }
                        return backend::UnknownRequestRespondRaw{*requestId, value.result};
                    },
                    [](const UnknownRequestReject& value) -> BackendCommandMapping {
                        const auto requestId = parsePendingRequestId(value.pendingRequestId);
                        if (!requestId.has_value()) {
                            return CommandMappingError{"pendingRequestId must be a non-zero unsigned decimal integer"};
                        }
                        return backend::UnknownRequestReject{*requestId, ProtocolError{value.code, value.message, value.data}};
                    }},
                parameters);
        }

    } // namespace

    struct FrontendConnection::Control {
        std::weak_ptr<BackendAdapter::Impl> adapter;
        std::uint64_t localId = 0;
        FrontendConnectionCallbacks callbacks;
        std::optional<backend::FrontendSession> backendSession;
        std::deque<OutboundMessage> outbound;
        std::unordered_set<std::string> pendingRequestIds;
        std::size_t outboundBytes = 0;
        bool open = true;
        bool helloDone = false;
        bool deliveryScheduled = false;
        bool closeAfterDelivery = false;
        bool closedNotified = false;
    };

    class BackendAdapter::Impl : public std::enable_shared_from_this<BackendAdapter::Impl> {
    public:
        Impl(backend::detail::BackendCoreRuntime& backend, BackendAdapterOptions options)
            : backendCore(&backend)
            , adapterOptions(std::move(options))
            , journal(adapterOptions.journal)
            , batchBuilder(adapterOptions.batches)
            , coalescer(adapterOptions.coalescer) {
            if (!adapterOptions.scheduler) {
                adapterOptions.scheduler = [](std::function<void()> callback) {
                    core::EventReceiver::atNextTick(callback);
                };
            }
        }

        void initialize() {
            const std::weak_ptr<Impl> weakSelf = shared_from_this();
            observer = backendCore->subscribe(
                backend::BackendObserverCallbacks{[weakSelf](const std::vector<backend::SequencedBackendEvent>& events) {
                                                      if (const std::shared_ptr<Impl> self = weakSelf.lock()) {
                                                          self->onBackendEvents(events);
                                                      }
                                                  },
                                                  [weakSelf](const backend::Snapshot& snapshot) {
                                                      if (const std::shared_ptr<Impl> self = weakSelf.lock()) {
                                                          self->onBackendResynchronize(snapshot);
                                                      }
                                                  }});
        }

        FrontendConnection openConnection(FrontendConnectionCallbacks callbacks) {
            if (!open) {
                return FrontendConnection{};
            }
            if (nextConnectionId == std::numeric_limits<std::uint64_t>::max()) {
                return FrontendConnection{};
            }
            auto control = std::make_shared<FrontendConnection::Control>();
            control->adapter = shared_from_this();
            control->localId = ++nextConnectionId;
            control->callbacks = std::move(callbacks);
            connections.emplace(control->localId, control);
            return FrontendConnection(std::move(control));
        }

        void schedule(std::function<void()> callback) noexcept {
            try {
                adapterOptions.scheduler(callback);
            } catch (...) {
                // Preserve ordered asynchronous delivery even when an
                // injected scheduler rejects a callback.
                try {
                    core::EventReceiver::atNextTick(std::move(callback));
                } catch (...) {
                }
            }
        }

        bool enqueue(const std::shared_ptr<FrontendConnection::Control>& control,
                     ServerMessage message,
                     bool closeAfterDelivery = false) noexcept {
            if (!open || !control || !control->open) {
                return false;
            }
            const auto serialized = Codec::serializeServer(message);
            if (!serialized) {
                closeControl(control, "frontend protocol serialization failed");
                return false;
            }
            const std::size_t size = serialized.value().size();
            if (adapterOptions.maxOutboundMessagesPerConnection == 0 ||
                control->outbound.size() >= adapterOptions.maxOutboundMessagesPerConnection ||
                size > adapterOptions.maxOutboundBytesPerConnection ||
                control->outboundBytes > adapterOptions.maxOutboundBytesPerConnection - size) {
                closeControl(control, "frontend outbound backpressure limit exceeded");
                return false;
            }

            try {
                control->outbound.push_back(OutboundMessage{std::move(message), serialized.value(), size});
                control->outboundBytes += size;
                control->closeAfterDelivery = control->closeAfterDelivery || closeAfterDelivery;
                if (!control->deliveryScheduled) {
                    control->deliveryScheduled = true;
                    scheduleDelivery(control);
                }
                return true;
            } catch (...) {
                closeControl(control, "frontend outbound queue allocation failed");
                return false;
            }
        }

        void scheduleDelivery(const std::shared_ptr<FrontendConnection::Control>& control) noexcept {
            const std::weak_ptr<Impl> weakSelf = shared_from_this();
            const std::weak_ptr<FrontendConnection::Control> weakControl = control;
            schedule([weakSelf, weakControl]() {
                const std::shared_ptr<Impl> self = weakSelf.lock();
                const std::shared_ptr<FrontendConnection::Control> locked = weakControl.lock();
                if (self && locked) {
                    self->deliver(locked);
                }
            });
        }

        void deliver(const std::shared_ptr<FrontendConnection::Control>& control) noexcept {
            if (!control->open) {
                return;
            }
            const std::size_t deliveryLimit = adapterOptions.maxMessagesPerDelivery;
            if (deliveryLimit == 0) {
                closeControl(control, "frontend delivery limit is zero");
                return;
            }

            std::size_t delivered = 0;
            while (control->open && !control->outbound.empty() && delivered < deliveryLimit) {
                OutboundMessage message = std::move(control->outbound.front());
                control->outbound.pop_front();
                control->outboundBytes -= message.serializedBytes;
                ++delivered;

                bool accepted = false;
                try {
                    accepted = control->callbacks.onMessage && control->callbacks.onMessage(message);
                } catch (...) {
                    closeControl(control, "frontend outbound callback threw");
                    return;
                }
                if (!accepted) {
                    closeControl(control, "frontend transport rejected outbound data");
                    return;
                }
            }

            if (!control->open) {
                return;
            }
            if (!control->outbound.empty()) {
                scheduleDelivery(control);
                return;
            }
            control->deliveryScheduled = false;
            if (control->closeAfterDelivery) {
                closeControl(control, "frontend protocol requested connection close");
            }
        }

        void closeControl(const std::shared_ptr<FrontendConnection::Control>& control, std::string reason) noexcept {
            if (!control || !control->open) {
                return;
            }
            control->open = false;
            control->outbound.clear();
            control->outboundBytes = 0;
            control->pendingRequestIds.clear();
            control->deliveryScheduled = false;
            control->closeAfterDelivery = false;
            if (control->backendSession.has_value()) {
                control->backendSession->close(reason);
                control->backendSession.reset();
            }
            connections.erase(control->localId);

            if (!control->closedNotified && control->callbacks.onClosed) {
                control->closedNotified = true;
                auto callback = control->callbacks.onClosed;
                schedule([callback = std::move(callback), reason = std::move(reason)]() {
                    try {
                        callback(reason);
                    } catch (...) {
                    }
                });
            }
        }

        void enqueueProtocolError(const std::shared_ptr<FrontendConnection::Control>& control,
                                  CodecError error,
                                  bool closeAfterDelivery) noexcept {
            ProtocolErrorMessage message;
            message.code = error.code;
            message.message = std::move(error.message);
            message.closeConnection = closeAfterDelivery || error.closeConnection;
            message.requestId = std::move(error.requestId);
            message.details = std::move(error.details);
            if (!error.supportedVersions.empty()) {
                message.supportedVersions = std::move(error.supportedVersions);
            } else {
                message.supportedVersions.assign(SupportedProtocolVersions.begin(), SupportedProtocolVersions.end());
            }
            const bool closes = message.closeConnection;
            enqueue(control, ServerMessage{std::move(message)}, closes);
        }

        ConnectionReceiveResult receiveError(const std::shared_ptr<FrontendConnection::Control>& control, CodecError error) noexcept {
            if (!control || !control->open) {
                return {ConnectionReceiveStatus::Closed, std::move(error)};
            }
            if (control->closeAfterDelivery) {
                return {ConnectionReceiveStatus::Closing, std::move(error)};
            }
            const bool closeConnection = !control->helloDone || error.closeConnection || error.code == ErrorCode::MalformedJson ||
                                         error.code == ErrorCode::WrongProtocol || error.code == ErrorCode::UnsupportedVersion;
            CodecError returned = error;
            enqueueProtocolError(control, std::move(error), closeConnection);
            return {closeConnection ? ConnectionReceiveStatus::Closing : ConnectionReceiveStatus::Rejected, std::move(returned)};
        }

        Snapshot frontendSnapshot(const backend::Snapshot* supplied = nullptr) const {
            const backend::Snapshot backendSnapshot = supplied != nullptr ? *supplied : backendCore->snapshot();
            return Snapshot{journal.currentSequence(),
                            backendSnapshotJson(backendSnapshot,
                                                journal.currentSequence(),
                                                journal.oldestReplayableAfter(),
                                                journal.oldestRetainedSequence(),
                                                journal.newestRetainedSequence()),
                            Json::object()};
        }

        void enqueueSnapshot(const std::shared_ptr<FrontendConnection::Control>& control,
                             const backend::Snapshot* supplied = nullptr) noexcept {
            try {
                enqueue(control, ServerMessage{frontendSnapshot(supplied)});
            } catch (...) {
                closeControl(control, "failed to create frontend snapshot");
            }
        }

        bool enqueueBatches(const std::shared_ptr<FrontendConnection::Control>& control,
                            const std::vector<FrontendEvent>& events) noexcept {
            const UpdateBatchResult built = batchBuilder.build(events);
            if (!built.success()) {
                return false;
            }
            for (const BoundedEventBatch& batch : built.batches) {
                if (!enqueue(control, ServerMessage{batch.batch})) {
                    return false;
                }
            }
            return true;
        }

        void synchronize(const std::shared_ptr<FrontendConnection::Control>& control, const Hello& hello) noexcept {
            if (!control->open || control->helloDone) {
                enqueueProtocolError(control,
                                     CodecError{ErrorCode::InvalidCommand,
                                                "hello has already completed for this connection",
                                                false,
                                                {},
                                                std::nullopt,
                                                std::nullopt},
                                     false);
                return;
            }

            flushNow();
            const std::weak_ptr<FrontendConnection::Control> weakControl = control;
            const std::weak_ptr<Impl> weakSelf = shared_from_this();
            try {
                control->backendSession.emplace(backendCore->openSession(
                    backend::FrontendSessionCallbacks{{},
                                                      [weakSelf, weakControl](const backend::Snapshot& snapshot) {
                                                          const auto self = weakSelf.lock();
                                                          const auto locked = weakControl.lock();
                                                          if (self && locked && locked->open) {
                                                              self->enqueueSnapshot(locked, &snapshot);
                                                          }
                                                      },
                                                      [weakSelf, weakControl](const backend::CommandCompletion& completion) {
                                                          const auto self = weakSelf.lock();
                                                          const auto locked = weakControl.lock();
                                                          if (self && locked && locked->open) {
                                                              self->onCommandCompleted(locked, completion);
                                                          }
                                                      },
                                                      [weakSelf, weakControl](const std::string& reason) {
                                                          const auto self = weakSelf.lock();
                                                          const auto locked = weakControl.lock();
                                                          if (self && locked && locked->open) {
                                                              self->closeControl(locked, reason);
                                                          }
                                                      }}));
            } catch (...) {
                enqueueProtocolError(
                    control,
                    CodecError{
                        ErrorCode::BackendUnavailable, "failed to open a backend frontend session", false, {}, std::nullopt, std::nullopt},
                    true);
                return;
            }
            if (!control->backendSession->isOpen()) {
                enqueueProtocolError(
                    control,
                    CodecError{
                        ErrorCode::BackendUnavailable, "backend frontend session is unavailable", false, {}, std::nullopt, std::nullopt},
                    true);
                return;
            }

            SyncMode syncMode = SyncMode::Snapshot;
            std::vector<FrontendEvent> replayEvents;
            bool replayUsable = false;
            if (hello.resumeAfter.has_value()) {
                try {
                    const JournalReplayResult replay = journal.replayAfter(*hello.resumeAfter);
                    if (replay.status == JournalReplayStatus::Available) {
                        const UpdateBatchResult built = batchBuilder.build(replay.events);
                        if (built.success()) {
                            syncMode = SyncMode::Replay;
                            replayEvents = replay.events;
                            replayUsable = true;
                        }
                    }
                } catch (...) {
                }
            }

            control->helloDone = true;
            const std::string id = std::to_string(control->backendSession->id().value());
            if (!enqueue(control, ServerMessage{Welcome{id, SessionRole::Observer, journal.currentSequence(), syncMode, Json::object()}})) {
                return;
            }
            if (replayUsable) {
                if (!enqueueBatches(control, replayEvents)) {
                    closeControl(control, "failed to enqueue frontend replay");
                    return;
                }
            } else {
                enqueueSnapshot(control);
            }
            enqueue(control, ServerMessage{SyncComplete{journal.currentSequence(), Json::object()}});
        }

        void enqueueFailure(const std::shared_ptr<FrontendConnection::Control>& control,
                            std::string requestId,
                            ErrorCode code,
                            std::string message,
                            std::optional<Json> details = std::nullopt) noexcept {
            enqueue(control,
                    ServerMessage{Response::failure(std::move(requestId),
                                                    CommandError{code, std::move(message), std::move(details), Json::object()})});
        }

        ConnectionReceiveResult receive(const std::shared_ptr<FrontendConnection::Control>& control,
                                        const ClientMessage& message) noexcept {
            if (!open || !control || !control->open) {
                return {ConnectionReceiveStatus::Closed, std::nullopt};
            }
            if (control->closeAfterDelivery) {
                return {ConnectionReceiveStatus::Closing, std::nullopt};
            }
            try {
                if (const auto* hello = std::get_if<Hello>(&message)) {
                    if (control->helloDone) {
                        CodecError error{ErrorCode::InvalidCommand, "hello may only be sent once", false, {}, std::nullopt, std::nullopt};
                        return receiveError(control, std::move(error));
                    }
                    synchronize(control, *hello);
                    return {control->open ? ConnectionReceiveStatus::Accepted : ConnectionReceiveStatus::Closing, std::nullopt};
                }

                const Command& command = std::get<Command>(message);
                if (!control->helloDone) {
                    CodecError error{
                        ErrorCode::InvalidCommand, "the first frontend message must be hello", true, {}, command.requestId, std::nullopt};
                    return receiveError(control, std::move(error));
                }
                if (control->pendingRequestIds.contains(command.requestId)) {
                    enqueueFailure(
                        control, command.requestId, ErrorCode::DuplicateRequestId, "requestId is already pending in this frontend session");
                    return {ConnectionReceiveStatus::Rejected, std::nullopt};
                }

                control->pendingRequestIds.insert(command.requestId);
                if (std::holds_alternative<SnapshotGet>(command.parameters)) {
                    flushNow();
                    control->pendingRequestIds.erase(command.requestId);
                    enqueue(control,
                            ServerMessage{Response::success(command.requestId, Json{{"sequence", journal.currentSequence().value()}})});
                    enqueueSnapshot(control);
                    enqueue(control, ServerMessage{SyncComplete{journal.currentSequence(), Json::object()}});
                    return {ConnectionReceiveStatus::Accepted, std::nullopt};
                }
                if (const auto* replay = std::get_if<ReplayAfter>(&command.parameters)) {
                    flushNow();
                    const JournalReplayResult replayResult = journal.replayAfter(replay->after);
                    control->pendingRequestIds.erase(command.requestId);
                    if (replayResult.status == JournalReplayStatus::FutureSequence) {
                        enqueueFailure(
                            control, command.requestId, ErrorCode::InvalidCommand, "events.replay cannot start after the current sequence");
                        return {ConnectionReceiveStatus::Rejected, std::nullopt};
                    }

                    SyncMode mode = SyncMode::Snapshot;
                    UpdateBatchResult built;
                    if (replayResult.status == JournalReplayStatus::Available) {
                        built = batchBuilder.build(replayResult.events);
                        if (built.success()) {
                            mode = SyncMode::Replay;
                        }
                    }
                    enqueue(control,
                            ServerMessage{Response::success(
                                command.requestId, Json{{"syncMode", toString(mode)}, {"sequence", journal.currentSequence().value()}})});
                    if (mode == SyncMode::Replay) {
                        for (const BoundedEventBatch& batch : built.batches) {
                            enqueue(control, ServerMessage{batch.batch});
                        }
                    } else {
                        enqueueSnapshot(control);
                    }
                    enqueue(control, ServerMessage{SyncComplete{journal.currentSequence(), Json::object()}});
                    return {ConnectionReceiveStatus::Accepted, std::nullopt};
                }

                const BackendCommandMapping mapping = mapBackendCommand(command.parameters);
                if (const auto* mappingError = std::get_if<CommandMappingError>(&mapping)) {
                    control->pendingRequestIds.erase(command.requestId);
                    enqueueFailure(control, command.requestId, ErrorCode::InvalidCommand, mappingError->message);
                    return {ConnectionReceiveStatus::Rejected, std::nullopt};
                }
                if (!control->backendSession.has_value() || !control->backendSession->isOpen()) {
                    control->pendingRequestIds.erase(command.requestId);
                    enqueueFailure(control, command.requestId, ErrorCode::BackendUnavailable, "backend frontend session is closed");
                    return {ConnectionReceiveStatus::Rejected, std::nullopt};
                }

                backend::CommandSubmission submission =
                    control->backendSession->submit(command.requestId, std::get<backend::BackendCommand>(mapping));
                if (!submission) {
                    control->pendingRequestIds.erase(command.requestId);
                    ErrorCode code = ErrorCode::LocalSubmissionFailure;
                    switch (submission.error) {
                        case backend::SubmissionError::None:
                        case backend::SubmissionError::Closed:
                            code = ErrorCode::BackendUnavailable;
                            break;
                        case backend::SubmissionError::EmptyRequestId:
                            code = ErrorCode::InvalidCommand;
                            break;
                        case backend::SubmissionError::DuplicateRequestId:
                            code = ErrorCode::DuplicateRequestId;
                            break;
                        case backend::SubmissionError::QueueFull:
                            code = ErrorCode::LocalSubmissionFailure;
                            break;
                    }
                    enqueueFailure(control, command.requestId, code, submission.message);
                    return {ConnectionReceiveStatus::Rejected, std::nullopt};
                }
                return {ConnectionReceiveStatus::Accepted, std::nullopt};
            } catch (const std::exception& exception) {
                CodecError error{ErrorCode::InternalError,
                                 std::string("failed to process frontend message: ") + exception.what(),
                                 false,
                                 {},
                                 std::nullopt,
                                 std::nullopt};
                return receiveError(control, std::move(error));
            } catch (...) {
                CodecError error{ErrorCode::InternalError,
                                 "failed to process frontend message: unknown local exception",
                                 false,
                                 {},
                                 std::nullopt,
                                 std::nullopt};
                return receiveError(control, std::move(error));
            }
        }

        Json commandValueJson(const backend::CommandValue& value) const {
            const backend::Snapshot currentSnapshot = backendCore->snapshot();
            return std::visit(
                Overloaded{[](const std::monostate&) {
                               return Json::object();
                           },
                           [this](const backend::Snapshot& snapshot) {
                               return backendSnapshotJson(snapshot,
                                                          journal.currentSequence(),
                                                          journal.oldestReplayableAfter(),
                                                          journal.oldestRetainedSequence(),
                                                          journal.newestRetainedSequence());
                           },
                           [](const backend::ControllerResult& result) {
                               Json encoded{{"role", backendRoleName(result.role)}};
                               if (result.controller.has_value()) {
                                   encoded["controllerSessionId"] = std::to_string(result.controller->value());
                               }
                               return encoded;
                           },
                           [](const backend::ReplayResult& result) {
                               return Json{{"backendSequence", result.after.value()}};
                           },
                           [&currentSnapshot](const typed::Thread& thread) {
                               const backend::ThreadSnapshot* found = findThread(currentSnapshot, thread.id.value);
                               return found != nullptr ? Json{{"thread", threadSnapshotJson(*found)}} : Json{{"threadId", thread.id.value}};
                           },
                           [&currentSnapshot](const typed::ThreadPage& page) {
                               Json encoded{{"threads", Json::array()}};
                               for (const typed::Thread& thread : page.data) {
                                   const backend::ThreadSnapshot* found = findThread(currentSnapshot, thread.id.value);
                                   encoded["threads"].push_back(found != nullptr ? threadSnapshotJson(*found)
                                                                                 : Json{{"id", thread.id.value}});
                               }
                               if (page.nextCursor.has_value()) {
                                   encoded["nextCursor"] = *page.nextCursor;
                               }
                               if (page.backwardsCursor.has_value()) {
                                   encoded["backwardsCursor"] = *page.backwardsCursor;
                               }
                               return encoded;
                           },
                           [&currentSnapshot](const typed::Turn& turn) {
                               const backend::TurnSnapshot* found = findTurn(currentSnapshot, turn.threadId.value, turn.id.value);
                               return found != nullptr ? Json{{"turn", turnSnapshotJson(*found)}} : Json{{"turnId", turn.id.value}};
                           },
                           [](const typed::TurnInterruptResult&) {
                               return Json::object();
                           }},
                value);
        }

        void onCommandCompleted(const std::shared_ptr<FrontendConnection::Control>& control,
                                const backend::CommandCompletion& completion) noexcept {
            if (!control->pendingRequestIds.erase(completion.requestId)) {
                return;
            }
            try {
                if (completion.result.error.has_value()) {
                    const backend::CommandError& error = *completion.result.error;
                    Json details = error.details;
                    if (error.remoteCode.has_value()) {
                        if (!details.is_object()) {
                            details = Json{{"details", details}};
                        }
                        details["remoteCode"] = *error.remoteCode;
                    }
                    enqueueFailure(control,
                                   completion.requestId,
                                   frontendErrorCode(error.code),
                                   error.message,
                                   details.is_null() ? std::nullopt : std::optional<Json>{std::move(details)});
                } else {
                    enqueue(control, ServerMessage{Response::success(completion.requestId, commandValueJson(completion.result.value))});
                }
            } catch (...) {
                enqueueFailure(control, completion.requestId, ErrorCode::InternalError, "failed to normalize backend command completion");
            }
        }

        CoalescerMarkResult
        markNormalized(CoalescingKey key, std::string type, Json data, FlushUrgency urgency = FlushUrgency::Deferred) noexcept {
            return coalescer.mark(DirtyUpdate{std::move(key), std::move(type), std::move(data), urgency});
        }

        Json lifecycleEventData(const backend::Snapshot& snapshot) const {
            Json data{{"lifecycle", backendLifecycleName(snapshot.lifecycle)}};
            if (snapshot.lastLifecycleError.has_value()) {
                data["error"] = errorSnapshotJson(*snapshot.lastLifecycleError);
            }
            return data;
        }

        Json threadListEventData(const backend::Snapshot& snapshot) const {
            Json data{{"hasLoadedPage", snapshot.threadList.hasLoadedPage},
                      {"complete", snapshot.threadList.complete},
                      {"pagesLoaded", snapshot.threadList.pagesLoaded}};
            if (snapshot.threadList.nextCursor.has_value()) {
                data["nextCursor"] = *snapshot.threadList.nextCursor;
            }
            if (snapshot.threadList.backwardsCursor.has_value()) {
                data["backwardsCursor"] = *snapshot.threadList.backwardsCursor;
            }
            return data;
        }

        CoalescerMarkResult
        markThread(const backend::Snapshot& snapshot, std::string_view threadId, FlushUrgency urgency = FlushUrgency::Deferred) noexcept {
            const backend::ThreadSnapshot* thread = findThread(snapshot, threadId);
            if (thread == nullptr) {
                return coalescer.requireSnapshot(urgency);
            }
            return markNormalized(
                CoalescingKey::thread(std::string(threadId)), "thread.updated", Json{{"thread", threadSnapshotJson(*thread)}}, urgency);
        }

        CoalescerMarkResult markTurn(const backend::Snapshot& snapshot,
                                     std::string_view threadId,
                                     std::string_view turnId,
                                     FlushUrgency urgency = FlushUrgency::Deferred) noexcept {
            const backend::TurnSnapshot* turn = findTurn(snapshot, threadId, turnId);
            if (turn == nullptr) {
                return coalescer.requireSnapshot(urgency);
            }
            return markNormalized(CoalescingKey::turn(std::string(threadId), std::string(turnId)),
                                  "turn.updated",
                                  Json{{"turn", turnSnapshotJson(*turn)}},
                                  urgency);
        }

        CoalescerMarkResult markItem(const backend::Snapshot& snapshot,
                                     std::string_view threadId,
                                     std::string_view turnId,
                                     std::string_view itemId,
                                     FlushUrgency urgency = FlushUrgency::Deferred) noexcept {
            const backend::ItemSnapshot* item = findItem(snapshot, threadId, turnId, itemId);
            if (item == nullptr) {
                return coalescer.requireSnapshot(urgency);
            }
            return markNormalized(CoalescingKey::item(std::string(threadId), std::string(turnId), std::string(itemId)),
                                  "item.updated",
                                  Json{{"threadId", threadId}, {"turnId", turnId}, {"item", itemSnapshotJson(*item)}},
                                  urgency);
        }

        CoalescerMarkResult markItemContent(const backend::Snapshot& snapshot, const backend::ItemContentChanged& content) noexcept {
            const backend::ItemSnapshot* item = findItem(snapshot, content.threadId.value, content.turnId.value, content.itemId.value);
            if (item == nullptr) {
                return coalescer.requireSnapshot();
            }
            std::string channel;
            std::string accumulated;
            switch (content.kind) {
                case backend::ItemContentChanged::Kind::AgentText:
                    channel = "agentText";
                    accumulated = item->agentText;
                    break;
                case backend::ItemContentChanged::Kind::ReasoningText:
                    channel = "reasoningText";
                    accumulated = item->reasoningText;
                    break;
                case backend::ItemContentChanged::Kind::ReasoningSummary:
                    channel = "reasoningSummary";
                    accumulated = item->reasoningSummary;
                    break;
                case backend::ItemContentChanged::Kind::CommandOutput:
                    channel = "commandOutput";
                    accumulated = item->commandOutput;
                    break;
            }
            return markNormalized(CoalescingKey::itemContent(content.threadId.value, content.turnId.value, content.itemId.value, channel),
                                  "item.content.updated",
                                  Json{{"threadId", content.threadId.value},
                                       {"turnId", content.turnId.value},
                                       {"itemId", content.itemId.value},
                                       {"channel", channel},
                                       {"content", accumulated},
                                       {"contentTruncated", item->contentTruncated},
                                       {"droppedContentBytes", item->droppedContentBytes}},
                                  FlushUrgency::Deferred);
        }

        bool normalizeEvent(const backend::SequencedBackendEvent& sequenced, const backend::Snapshot& snapshot) noexcept {
            CoalescerMarkResult result;
            std::visit(
                Overloaded{
                    [&](const backend::LifecycleChanged&) {
                        result = markNormalized(CoalescingKey{DirtyEntityKind::BackendLifecycle, {}, {}, {}, {}},
                                                "backend.lifecycle.changed",
                                                lifecycleEventData(snapshot),
                                                FlushUrgency::Immediate);
                    },
                    [&](const backend::DiagnosticReceived&) {
                        result = markNormalized(CoalescingKey{DirtyEntityKind::Diagnostic, {}, {}, {}, {}},
                                                "diagnostics.updated",
                                                Json{{"received", snapshot.diagnostics.received}, {"recent", snapshot.diagnostics.recent}});
                    },
                    [&](const backend::ThreadUpserted& event) {
                        result = markThread(snapshot, event.thread.id.value);
                    },
                    [&](const backend::ThreadListUpdated& event) {
                        for (const typed::Thread& thread : event.page.data) {
                            const CoalescerMarkResult threadResult = markThread(snapshot, thread.id.value);
                            if (threadResult.immediateFlush) {
                                result = threadResult;
                            }
                        }
                        const CoalescerMarkResult listResult =
                            markNormalized(CoalescingKey{DirtyEntityKind::Thread, {}, {}, "thread-list", {}},
                                           "thread.list.updated",
                                           threadListEventData(snapshot));
                        if (listResult.status != CoalescerMarkStatus::Accepted) {
                            result = listResult;
                        }
                    },
                    [&](const backend::ThreadStatusUpdated& event) {
                        result = markThread(snapshot, event.threadId.value);
                    },
                    [&](const backend::TurnUpserted& event) {
                        const backend::TurnSnapshot* turn = findTurn(snapshot, event.turn.threadId.value, event.turn.id.value);
                        result = markTurn(snapshot,
                                          event.turn.threadId.value,
                                          event.turn.id.value,
                                          turn != nullptr && turn->terminal ? FlushUrgency::Immediate : FlushUrgency::Deferred);
                    },
                    [&](const backend::TurnCompleted& event) {
                        result = markTurn(snapshot, event.turn.threadId.value, event.turn.id.value, FlushUrgency::Immediate);
                    },
                    [&](const backend::TurnFailed& event) {
                        result = markTurn(snapshot, event.turn.threadId.value, event.turn.id.value, FlushUrgency::Immediate);
                    },
                    [&](const backend::TurnErrorUpdated& event) {
                        result = markTurn(snapshot,
                                          event.threadId.value,
                                          event.turnId.value,
                                          event.willRetry ? FlushUrgency::Deferred : FlushUrgency::Immediate);
                    },
                    [&](const backend::ItemUpserted& event) {
                        const auto id = backend::itemId(event.item);
                        if (!id.has_value()) {
                            result = coalescer.requireSnapshot(event.lifecycle == backend::ItemLifecycle::Completed ||
                                                                       event.lifecycle == backend::ItemLifecycle::Failed
                                                                   ? FlushUrgency::Immediate
                                                                   : FlushUrgency::Deferred);
                            return;
                        }
                        const bool terminal =
                            event.lifecycle == backend::ItemLifecycle::Completed || event.lifecycle == backend::ItemLifecycle::Failed;
                        result = markItem(snapshot,
                                          event.threadId.value,
                                          event.turnId.value,
                                          id->value,
                                          terminal ? FlushUrgency::Immediate : FlushUrgency::Deferred);
                    },
                    [&](const backend::ItemContentChanged& event) {
                        result = markItemContent(snapshot, event);
                    },
                    [&](const backend::FileChangeUpdated& event) {
                        result = markItem(snapshot, event.threadId.value, event.turnId.value, event.itemId.value);
                    },
                    [&](const backend::TokenUsageUpdated& event) {
                        result = markTurn(snapshot, event.threadId.value, event.turnId.value);
                    },
                    [&](const backend::ModelRerouted& event) {
                        result = markTurn(snapshot, event.threadId.value, event.turnId.value);
                    },
                    [&](const backend::PendingRequestAdded& event) {
                        const backend::PendingRequestSnapshot* pending = findPending(snapshot, event.pending.id);
                        if (pending == nullptr) {
                            result = coalescer.requireSnapshot(FlushUrgency::Immediate);
                            return;
                        }
                        result = markNormalized(CoalescingKey::pendingRequest(std::to_string(event.pending.id.value())),
                                                "request.pending",
                                                Json{{"request", pendingRequestSnapshotJson(*pending)}},
                                                FlushUrgency::Immediate);
                    },
                    [&](const backend::PendingRequestRemoved& event) {
                        result = markNormalized(CoalescingKey::pendingRequest(std::to_string(event.id.value())),
                                                "request.resolved",
                                                Json{{"pendingRequestId", std::to_string(event.id.value())}, {"reason", event.reason}},
                                                FlushUrgency::Immediate);
                    },
                    [&](const backend::ControllerChanged& event) {
                        Json data = Json::object();
                        if (event.controller.has_value()) {
                            data["controllerSessionId"] = std::to_string(event.controller->value());
                        }
                        result = markNormalized(CoalescingKey{DirtyEntityKind::Controller, {}, {}, {}, {}},
                                                "controller.changed",
                                                std::move(data),
                                                FlushUrgency::Immediate);
                    },
                    [&](const backend::SessionChanged& event) {
                        result = markNormalized(CoalescingKey{DirtyEntityKind::Session, {}, {}, std::to_string(event.id.value()), {}},
                                                "session.changed",
                                                Json{{"sessionId", std::to_string(event.id.value())},
                                                     {"connected", event.connected},
                                                     {"role", backendRoleName(event.role)}},
                                                FlushUrgency::Immediate);
                    },
                    [&](const backend::CodexExtensionReceived& event) {
                        try {
                            const backend::ExtensionSnapshot extension = backend::makeExtensionSnapshot(backend::ExtensionRecord{
                                event.method, event.payload, event.decodingError, std::nullopt, std::nullopt, std::nullopt});
                            Json data = extensionSnapshotJson(extension);
                            result = markNormalized(
                                CoalescingKey{
                                    DirtyEntityKind::CodexExtension, {}, {}, std::to_string(sequenced.sequence.value()), extension.method},
                                "codex.extension",
                                std::move(data));
                        } catch (...) {
                            // Snapshot synchronization still exposes the
                            // reducer-retained sanitized extension record.
                            result = coalescer.requireSnapshot(FlushUrgency::Immediate);
                        }
                    }},
                sequenced.event);
            return result.immediateFlush || result.status == CoalescerMarkStatus::SnapshotRequired ||
                   result.status == CoalescerMarkStatus::AllocationFailure;
        }

        void onBackendEvents(const std::vector<backend::SequencedBackendEvent>& events) noexcept {
            if (!open || events.empty()) {
                return;
            }
            try {
                const backend::Snapshot snapshot = backendCore->snapshot();
                bool immediate = false;
                for (const backend::SequencedBackendEvent& event : events) {
                    immediate = normalizeEvent(event, snapshot) || immediate;
                }
                if (immediate) {
                    flushNow();
                } else {
                    scheduleFlush();
                }
            } catch (...) {
                (void) coalescer.requireSnapshot(FlushUrgency::Immediate);
                flushNow();
            }
        }

        void onBackendResynchronize(const backend::Snapshot& snapshot) noexcept {
            if (!open) {
                return;
            }
            coalescer.clear();
            if (!journal.invalidateReplay()) {
                sequenceExhausted = true;
            }
            Json state = backendSnapshotJson(snapshot,
                                             journal.currentSequence(),
                                             journal.oldestReplayableAfter(),
                                             journal.oldestRetainedSequence(),
                                             journal.newestRetainedSequence());
            state["frontendSequenceExhausted"] = sequenceExhausted;
            broadcast(ServerMessage{Snapshot{journal.currentSequence(), std::move(state), Json::object()}});
        }

        void scheduleFlush() noexcept {
            if (!open || flushCallbackPending || !coalescer.flushScheduled()) {
                return;
            }
            flushCallbackPending = true;
            const std::weak_ptr<Impl> weakSelf = shared_from_this();
            schedule([weakSelf]() {
                if (const auto self = weakSelf.lock()) {
                    self->flushCallbackPending = false;
                    self->flushNow();
                }
            });
        }

        void broadcast(const ServerMessage& message) noexcept {
            std::vector<std::shared_ptr<FrontendConnection::Control>> recipients;
            try {
                recipients.reserve(connections.size());
                for (const auto& [id, control] : connections) {
                    (void) id;
                    if (control->open && control->helloDone) {
                        recipients.push_back(control);
                    }
                }
            } catch (...) {
                return;
            }
            for (const auto& control : recipients) {
                enqueue(control, message);
            }
        }

        void broadcastSnapshot() noexcept {
            try {
                Snapshot snapshot = frontendSnapshot();
                snapshot.state["frontendSequenceExhausted"] = sequenceExhausted;
                broadcast(ServerMessage{std::move(snapshot)});
            } catch (...) {
            }
        }

        void flushNow() noexcept {
            if (!open) {
                return;
            }
            if (flushing) {
                flushAgain = true;
                return;
            }
            flushing = true;
            do {
                flushAgain = false;
                try {
                    CoalescerDrainResult dirty = coalescer.drain();
                    if (dirty.snapshotRequired || sequenceExhausted) {
                        if (!journal.invalidateReplay()) {
                            sequenceExhausted = true;
                        }
                        broadcastSnapshot();
                        continue;
                    }

                    std::vector<FrontendEvent> events;
                    events.reserve(dirty.updates.size());
                    bool snapshotFallback = false;
                    for (DirtyUpdate& update : dirty.updates) {
                        JournalAppendResult appended = journal.append(std::move(update.type), std::move(update.data), Json::object());
                        if (appended.status == JournalAppendStatus::SequenceOverflow) {
                            sequenceExhausted = true;
                            (void) journal.invalidateReplay();
                            ProtocolErrorMessage error;
                            error.code = ErrorCode::SequenceOverflow;
                            error.message = "frontend event sequence is exhausted";
                            error.supportedVersions.assign(SupportedProtocolVersions.begin(), SupportedProtocolVersions.end());
                            broadcast(ServerMessage{std::move(error)});
                            snapshotFallback = true;
                            break;
                        }
                        if (!appended.accepted() || !appended.event.has_value()) {
                            if (!journal.invalidateReplay()) {
                                sequenceExhausted = true;
                            }
                            snapshotFallback = true;
                            break;
                        }
                        events.push_back(std::move(*appended.event));
                    }
                    if (snapshotFallback) {
                        broadcastSnapshot();
                        continue;
                    }
                    if (events.empty()) {
                        continue;
                    }

                    const UpdateBatchResult built = batchBuilder.build(events);
                    if (!built.success()) {
                        if (!journal.invalidateReplay()) {
                            sequenceExhausted = true;
                        }
                        broadcastSnapshot();
                        continue;
                    }
                    for (const BoundedEventBatch& batch : built.batches) {
                        broadcast(ServerMessage{batch.batch});
                    }
                } catch (...) {
                    coalescer.clear();
                    if (!journal.invalidateReplay()) {
                        sequenceExhausted = true;
                    }
                    broadcastSnapshot();
                }
            } while (flushAgain || coalescer.flushScheduled());
            flushing = false;
        }

        void shutdown(std::string reason) noexcept {
            if (!open) {
                return;
            }
            open = false;
            observer.close();
            coalescer.clear();

            std::vector<std::shared_ptr<FrontendConnection::Control>> active;
            try {
                active.reserve(connections.size());
                for (const auto& [id, control] : connections) {
                    (void) id;
                    active.push_back(control);
                }
            } catch (...) {
            }
            for (const auto& control : active) {
                closeControl(control, reason);
            }
            connections.clear();
        }

        bool isOpen() const noexcept {
            return open;
        }

        bool isFlushScheduled() const noexcept {
            return flushCallbackPending || coalescer.flushScheduled();
        }

        std::size_t connectionCount() const noexcept {
            return connections.size();
        }

        backend::detail::BackendCoreRuntime* backendCore;
        BackendAdapterOptions adapterOptions;
        EventJournal journal;
        UpdateBatchBuilder batchBuilder;
        EventCoalescer coalescer;
        backend::BackendObserverSubscription observer;
        std::map<std::uint64_t, std::shared_ptr<FrontendConnection::Control>> connections;
        std::uint64_t nextConnectionId = 0;
        bool open = true;
        bool flushCallbackPending = false;
        bool flushing = false;
        bool flushAgain = false;
        bool sequenceExhausted = false;
    };

    FrontendConnection::FrontendConnection() noexcept = default;

    FrontendConnection::FrontendConnection(std::shared_ptr<Control> control) noexcept
        : control(std::move(control)) {
    }

    FrontendConnection::FrontendConnection(FrontendConnection&& other) noexcept
        : control(std::move(other.control)) {
    }

    FrontendConnection& FrontendConnection::operator=(FrontendConnection&& other) noexcept {
        if (this != &other) {
            close();
            control = std::move(other.control);
        }
        return *this;
    }

    FrontendConnection::~FrontendConnection() {
        close();
    }

    ConnectionReceiveResult FrontendConnection::receive(const ClientMessage& message) noexcept {
        if (!control || !control->open) {
            return {ConnectionReceiveStatus::Closed, std::nullopt};
        }
        if (control->closeAfterDelivery) {
            return {ConnectionReceiveStatus::Closing, std::nullopt};
        }
        const auto adapter = control->adapter.lock();
        if (!adapter) {
            control->open = false;
            return {ConnectionReceiveStatus::Closed, std::nullopt};
        }
        return adapter->receive(control, message);
    }

    ConnectionReceiveResult FrontendConnection::receive(const Json& message) noexcept {
        if (!control || !control->open) {
            return {ConnectionReceiveStatus::Closed, std::nullopt};
        }
        if (control->closeAfterDelivery) {
            return {ConnectionReceiveStatus::Closing, std::nullopt};
        }
        const auto decoded = Codec::decodeClient(message);
        if (!decoded) {
            return receiveError(decoded.error());
        }
        return receive(decoded.value());
    }

    ConnectionReceiveResult FrontendConnection::receive(std::string_view compactJson) noexcept {
        if (!control || !control->open) {
            return {ConnectionReceiveStatus::Closed, std::nullopt};
        }
        if (control->closeAfterDelivery) {
            return {ConnectionReceiveStatus::Closing, std::nullopt};
        }
        const auto decoded = Codec::decodeClient(compactJson);
        if (!decoded) {
            return receiveError(decoded.error());
        }
        return receive(decoded.value());
    }

    ConnectionReceiveResult FrontendConnection::receiveError(CodecError error) noexcept {
        if (!control || !control->open) {
            return {ConnectionReceiveStatus::Closed, std::move(error)};
        }
        if (control->closeAfterDelivery) {
            return {ConnectionReceiveStatus::Closing, std::move(error)};
        }
        const auto adapter = control->adapter.lock();
        if (!adapter) {
            control->open = false;
            return {ConnectionReceiveStatus::Closed, std::move(error)};
        }
        return adapter->receiveError(control, std::move(error));
    }

    void FrontendConnection::close(std::string reason) noexcept {
        if (!control) {
            return;
        }
        if (const auto adapter = control->adapter.lock()) {
            adapter->closeControl(control, std::move(reason));
        } else {
            control->open = false;
            control->outbound.clear();
            control->outboundBytes = 0;
            control->backendSession.reset();
        }
        control.reset();
    }

    bool FrontendConnection::isOpen() const noexcept {
        return control && control->open;
    }

    bool FrontendConnection::helloComplete() const noexcept {
        return control && control->open && control->helloDone;
    }

    std::optional<std::string> FrontendConnection::sessionId() const {
        if (!control || !control->open || !control->backendSession.has_value()) {
            return std::nullopt;
        }
        return std::to_string(control->backendSession->id().value());
    }

    std::size_t FrontendConnection::queuedMessages() const noexcept {
        return control ? control->outbound.size() : 0;
    }

    std::size_t FrontendConnection::queuedBytes() const noexcept {
        return control ? control->outboundBytes : 0;
    }

    BackendAdapter::BackendAdapter(backend::detail::BackendCoreRuntime& backend, BackendAdapterOptions options)
        : impl(std::make_shared<Impl>(backend, std::move(options))) {
        impl->initialize();
    }

    BackendAdapter::~BackendAdapter() {
        close();
    }

    FrontendConnection BackendAdapter::openConnection(FrontendConnectionCallbacks callbacks) {
        return impl ? impl->openConnection(std::move(callbacks)) : FrontendConnection{};
    }

    void BackendAdapter::flush() {
        if (impl) {
            impl->flushNow();
        }
    }

    void BackendAdapter::close(std::string reason) noexcept {
        if (impl) {
            impl->shutdown(std::move(reason));
            impl.reset();
        }
    }

    bool BackendAdapter::isOpen() const noexcept {
        return impl && impl->isOpen();
    }

    bool BackendAdapter::flushScheduled() const noexcept {
        return impl && impl->isFlushScheduled();
    }

    SequenceNumber BackendAdapter::currentSequence() const noexcept {
        return impl ? impl->journal.currentSequence() : SequenceNumber{};
    }

    std::size_t BackendAdapter::connectionCount() const noexcept {
        return impl ? impl->connectionCount() : 0;
    }

    EventJournalConfig BackendAdapter::journalConfig() const noexcept {
        return impl ? impl->journal.config() : EventJournalConfig{};
    }

    UpdateBatchConfig BackendAdapter::batchConfig() const noexcept {
        return impl ? impl->batchBuilder.config() : UpdateBatchConfig{};
    }

} // namespace ai::openai::codex::frontend
