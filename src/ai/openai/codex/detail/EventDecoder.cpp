/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/EventDecoder.h"

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/CodexErrorInfoCodec.h"
#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/detail/ItemDecoder.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/ThreadCodec.h"
#include "ai/openai/codex/detail/TurnCodec.h"

#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace ai::openai::codex::detail {

    namespace {

        const Json* findMember(const Json& object, const char* name) {
            if (!object.is_object()) {
                return nullptr;
            }

            const auto member = object.find(name);
            return member == object.end() ? nullptr : &*member;
        }

        bool requireParamsObject(const Notification& notification, std::string& error) {
            if (!notification.params.is_object()) {
                error = "notification params must be an object";
                return false;
            }
            return true;
        }

        bool requireString(const Json& object, const char* name, std::string& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }
            if (!member->is_string()) {
                error = std::string("notification param '") + name + "' must be a string";
                return false;
            }

            value = member->get<std::string>();
            return true;
        }

        bool requireBoolean(const Json& object, const char* name, bool& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }
            if (!member->is_boolean()) {
                error = std::string("notification param '") + name + "' must be a boolean";
                return false;
            }

            value = member->get<bool>();
            return true;
        }

        bool requireObject(const Json& object, const char* name, const Json*& value, std::string& error) {
            value = findMember(object, name);
            if (value == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }
            if (!value->is_object()) {
                error = std::string("notification param '") + name + "' must be an object";
                return false;
            }
            return true;
        }

        bool requireArray(const Json& object, const char* name, const Json*& value, std::string& error) {
            value = findMember(object, name);
            if (value == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }
            if (!value->is_array()) {
                error = std::string("notification param '") + name + "' must be an array";
                return false;
            }
            return true;
        }

        bool requireInt64(const Json& object, const char* name, std::int64_t& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }

            if (member->is_number_unsigned()) {
                const std::uint64_t integer = member->get<std::uint64_t>();
                if (integer > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    error = std::string("notification param '") + name + "' is outside the int64 range";
                    return false;
                }
                value = static_cast<std::int64_t>(integer);
                return true;
            }
            if (member->is_number_integer()) {
                value = member->get<std::int64_t>();
                return true;
            }

            error = std::string("notification param '") + name + "' must be an integer";
            return false;
        }

        typed::Event unknownEvent(const Notification& notification, std::optional<std::string> decodingError = std::nullopt) {
            const bool malformed = decodingError.has_value();
            return typed::Event{typed::UnknownEvent{
                notification.method,
                notification.params,
                notification.raw,
                std::move(decodingError),
                malformed ? std::optional<typed::DecodeDiagnostic>{
                                malformedKnownDiagnostic(notification.method, "$.params")}
                          : std::optional<typed::DecodeDiagnostic>{
                                unknownMethodDiagnostic(notification.method)}}};
        }

        typed::Event malformedEvent(const Notification& notification, std::string error) {
            if (error.empty()) {
                error = "notification payload could not be decoded";
            }
            return unknownEvent(notification, notification.method + ": " + std::move(error));
        }

        std::optional<typed::ThreadStatus> decodeThreadStatus(const Json& value, std::string& error) {
            if (!value.is_object()) {
                error = "notification param 'status' must be an object";
                return std::nullopt;
            }

            std::string type;
            if (!requireString(value, "type", type, error)) {
                return std::nullopt;
            }
            if (type == "active") {
                const Json* activeFlags = nullptr;
                if (!requireArray(value, "activeFlags", activeFlags, error)) {
                    return std::nullopt;
                }
            }

            return typed::ThreadStatus{std::move(type), value};
        }

        typed::Event decodeThreadStarted(const Notification& notification) {
            std::string error;
            const Json* threadValue = nullptr;
            if (!requireParamsObject(notification, error) || !requireObject(notification.params, "thread", threadValue, error)) {
                return malformedEvent(notification, std::move(error));
            }

            auto thread = decodeThread(*threadValue, error);
            if (!thread.has_value()) {
                return malformedEvent(notification, "thread: " + std::move(error));
            }
            return typed::Event{typed::ThreadStarted{std::move(*thread), notification.raw}};
        }

        typed::Event decodeThreadStatusChanged(const Notification& notification) {
            std::string error;
            std::string threadId;
            const Json* statusValue = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !requireObject(notification.params, "status", statusValue, error)) {
                return malformedEvent(notification, std::move(error));
            }

            auto status = decodeThreadStatus(*statusValue, error);
            if (!status.has_value()) {
                return malformedEvent(notification, std::move(error));
            }
            return typed::Event{typed::ThreadStatusChanged{typed::ThreadId{std::move(threadId)}, std::move(*status), notification.raw}};
        }

        typed::Event decodeTurnStarted(const Notification& notification) {
            std::string error;
            std::string threadIdValue;
            const Json* turnValue = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadIdValue, error) ||
                !requireObject(notification.params, "turn", turnValue, error)) {
                return malformedEvent(notification, std::move(error));
            }

            const typed::ThreadId threadId{std::move(threadIdValue)};
            auto turn = decodeTurn(*turnValue, threadId, error);
            if (!turn.has_value()) {
                return malformedEvent(notification, "turn: " + std::move(error));
            }
            return typed::Event{typed::TurnStarted{std::move(*turn), notification.raw}};
        }

        typed::Event decodeTurnCompleted(const Notification& notification) {
            std::string error;
            std::string threadIdValue;
            const Json* turnValue = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadIdValue, error) ||
                !requireObject(notification.params, "turn", turnValue, error)) {
                return malformedEvent(notification, std::move(error));
            }

            const typed::ThreadId threadId{std::move(threadIdValue)};
            auto turn = decodeTurn(*turnValue, threadId, error);
            if (!turn.has_value()) {
                return malformedEvent(notification, "turn: " + std::move(error));
            }
            if (turn->status.value == "failed") {
                Json turnError = turn->error.has_value() ? *turn->error : Json(nullptr);
                return typed::Event{typed::TurnFailed{std::move(*turn), std::move(turnError), notification.raw}};
            }
            return typed::Event{typed::TurnCompleted{std::move(*turn), notification.raw}};
        }

        typed::Event decodeItemLifecycle(const Notification& notification, bool started) {
            std::string error;
            std::string threadIdValue;
            std::string turnIdValue;
            const Json* itemValue = nullptr;
            const char* timestampName = started ? "startedAtMs" : "completedAtMs";
            std::int64_t timestamp = 0;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadIdValue, error) ||
                !requireString(notification.params, "turnId", turnIdValue, error) ||
                !requireObject(notification.params, "item", itemValue, error) ||
                !requireInt64(notification.params, timestampName, timestamp, error)) {
                return malformedEvent(notification, std::move(error));
            }

            const typed::ThreadId threadId{std::move(threadIdValue)};
            const typed::TurnId turnId{std::move(turnIdValue)};
            auto item = decodeItem(*itemValue, threadId, turnId, error);
            if (!item.has_value()) {
                return malformedEvent(notification, "item: " + std::move(error));
            }
            if (started) {
                return typed::Event{typed::ItemStarted{std::move(*item), timestamp, notification.raw}};
            }
            return typed::Event{typed::ItemCompleted{std::move(*item), timestamp, notification.raw}};
        }

        bool decodeDeltaMetadata(const Notification& notification,
                                 std::string& threadId,
                                 std::string& turnId,
                                 std::string& itemId,
                                 std::string& delta,
                                 std::string& error) {
            return requireParamsObject(notification, error) && requireString(notification.params, "threadId", threadId, error) &&
                   requireString(notification.params, "turnId", turnId, error) &&
                   requireString(notification.params, "itemId", itemId, error) && requireString(notification.params, "delta", delta, error);
        }

        typed::Event decodeAgentMessageDelta(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            std::string delta;
            if (!decodeDeltaMetadata(notification, threadId, turnId, itemId, delta, error)) {
                return malformedEvent(notification, std::move(error));
            }

            return typed::Event{typed::AgentMessageDelta{typed::ThreadId{std::move(threadId)},
                                                         typed::TurnId{std::move(turnId)},
                                                         typed::ItemId{std::move(itemId)},
                                                         std::move(delta),
                                                         notification.raw}};
        }

        typed::Event decodeReasoningDelta(const Notification& notification, typed::ReasoningDelta::Kind kind) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            std::string delta;
            std::int64_t index = 0;
            const char* indexName = kind == typed::ReasoningDelta::Kind::Text ? "contentIndex" : "summaryIndex";
            if (!decodeDeltaMetadata(notification, threadId, turnId, itemId, delta, error) ||
                !requireInt64(notification.params, indexName, index, error)) {
                return malformedEvent(notification, std::move(error));
            }

            return typed::Event{typed::ReasoningDelta{typed::ThreadId{std::move(threadId)},
                                                      typed::TurnId{std::move(turnId)},
                                                      typed::ItemId{std::move(itemId)},
                                                      std::move(delta),
                                                      kind,
                                                      index,
                                                      notification.raw}};
        }

        typed::Event decodeCommandOutputDelta(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            std::string delta;
            if (!decodeDeltaMetadata(notification, threadId, turnId, itemId, delta, error)) {
                return malformedEvent(notification, std::move(error));
            }

            return typed::Event{typed::CommandOutputDelta{typed::ThreadId{std::move(threadId)},
                                                          typed::TurnId{std::move(turnId)},
                                                          typed::ItemId{std::move(itemId)},
                                                          std::move(delta),
                                                          notification.raw}};
        }

        typed::Event decodeFileChangeUpdated(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            const Json* changes = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error) ||
                !requireString(notification.params, "itemId", itemId, error) ||
                !requireArray(notification.params, "changes", changes, error)) {
                return malformedEvent(notification, std::move(error));
            }

            return typed::Event{typed::FileChangeUpdated{typed::ThreadId{std::move(threadId)},
                                                         typed::TurnId{std::move(turnId)},
                                                         typed::ItemId{std::move(itemId)},
                                                         *changes,
                                                         notification.raw}};
        }

        typed::Event decodeTokenUsageUpdated(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            const Json* tokenUsage = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error) ||
                !requireObject(notification.params, "tokenUsage", tokenUsage, error)) {
                return malformedEvent(notification, std::move(error));
            }

            return typed::Event{typed::TokenUsageUpdated{
                typed::ThreadId{std::move(threadId)}, typed::TurnId{std::move(turnId)}, *tokenUsage, notification.raw}};
        }

        typed::Event decodeModelRerouted(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string fromModel;
            std::string toModel;
            std::string reason;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error) ||
                !requireString(notification.params, "fromModel", fromModel, error) ||
                !requireString(notification.params, "toModel", toModel, error) ||
                !requireString(notification.params, "reason", reason, error)) {
                return malformedEvent(notification, std::move(error));
            }

            return typed::Event{typed::ModelRerouted{typed::ThreadId{std::move(threadId)},
                                                     typed::TurnId{std::move(turnId)},
                                                     typed::ModelId{std::move(fromModel)},
                                                     typed::ModelId{std::move(toModel)},
                                                     std::move(reason),
                                                     notification.raw}};
        }

        typed::Event decodeTurnError(const Notification& notification) {
            std::string decodingError;
            std::string threadId;
            std::string turnId;
            const Json* turnError = nullptr;
            bool willRetry = false;
            if (!requireParamsObject(notification, decodingError) ||
                !requireString(notification.params, "threadId", threadId, decodingError) ||
                !requireString(notification.params, "turnId", turnId, decodingError) ||
                !requireObject(notification.params, "error", turnError, decodingError) ||
                !requireBoolean(notification.params, "willRetry", willRetry, decodingError)) {
                return malformedEvent(notification, std::move(decodingError));
            }

            std::optional<typed::DecodeDiagnostic> turnDiagnostic;
            std::optional<typed::TurnError> typedError = detail::decodeTurnError(*turnError, turnDiagnostic);
            if (!typedError) {
                return typed::Event{typed::UnknownEvent{notification.method,
                                                        notification.params,
                                                        notification.raw,
                                                        notification.method + ": error payload could not be decoded",
                                                        std::move(turnDiagnostic)}};
            }
            return typed::Event{typed::TurnErrorEvent{
                typed::ThreadId{std::move(threadId)},
                typed::TurnId{std::move(turnId)},
                *turnError,
                willRetry,
                notification.raw,
                std::move(typedError)}};
        }

    } // namespace

    typed::Event decodeEvent(const Notification& notification) noexcept {
        try {
            const ProtocolSurfaceEntry* entry =
                findSurface(SurfaceCategory::ServerNotification, "ServerNotification", "method", notification.method);
            const ServerNotificationTarget* target = entry == nullptr || entry->runtimeDisposition != RuntimeDisposition::Typed
                                                         ? nullptr
                                                         : std::get_if<ServerNotificationTarget>(&entry->runtimeTarget);
            if (target == nullptr) {
                return unknownEvent(notification);
            }

            switch (*target) {
                case ServerNotificationTarget::Error:
                    return decodeTurnError(notification);
                case ServerNotificationTarget::ThreadStarted:
                    return decodeThreadStarted(notification);
                case ServerNotificationTarget::ThreadStatusChanged:
                    return decodeThreadStatusChanged(notification);
                case ServerNotificationTarget::TurnStarted:
                    return decodeTurnStarted(notification);
                case ServerNotificationTarget::TurnCompleted:
                    return decodeTurnCompleted(notification);
                case ServerNotificationTarget::ItemStarted:
                    return decodeItemLifecycle(notification, true);
                case ServerNotificationTarget::ItemCompleted:
                    return decodeItemLifecycle(notification, false);
                case ServerNotificationTarget::AgentMessageDelta:
                    return decodeAgentMessageDelta(notification);
                case ServerNotificationTarget::ReasoningTextDelta:
                    return decodeReasoningDelta(notification, typed::ReasoningDelta::Kind::Text);
                case ServerNotificationTarget::ReasoningSummaryTextDelta:
                    return decodeReasoningDelta(notification, typed::ReasoningDelta::Kind::Summary);
                case ServerNotificationTarget::CommandExecutionOutputDelta:
                    return decodeCommandOutputDelta(notification);
                case ServerNotificationTarget::FileChangePatchUpdated:
                    return decodeFileChangeUpdated(notification);
                case ServerNotificationTarget::ThreadTokenUsageUpdated:
                    return decodeTokenUsageUpdated(notification);
                case ServerNotificationTarget::ModelRerouted:
                    return decodeModelRerouted(notification);
                case ServerNotificationTarget::Count:
                    break;
            }

            return unknownEvent(notification);
        } catch (const std::exception& exception) {
            try {
                return unknownEvent(notification, std::string("event decoding failed: ") + exception.what());
            } catch (...) {
                return typed::Event{typed::UnknownEvent{
                    "", nullptr, nullptr, "event decoding failed", malformedKnownDiagnostic("ServerNotification", "$")}};
            }
        } catch (...) {
            try {
                return unknownEvent(notification, "event decoding failed with an unknown exception");
            } catch (...) {
                return typed::Event{typed::UnknownEvent{
                    "", nullptr, nullptr, "event decoding failed", malformedKnownDiagnostic("ServerNotification", "$")}};
            }
        }
    }

} // namespace ai::openai::codex::detail
