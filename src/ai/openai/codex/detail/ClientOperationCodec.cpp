/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ClientOperationCodec.h"

#include "ai/openai/codex/detail/ThreadCodec.h"
#include "ai/openai/codex/detail/TurnCodec.h"

#include <array>
#include <cstddef>
#include <exception>
#include <span>
#include <utility>

namespace ai::openai::codex::detail {

    namespace {
        constexpr std::array<std::string_view, static_cast<std::size_t>(ClientOperationResultDecoder::Count)> ResultDecoderIdentities{{
            "Unit",
            "ThreadForkResponse",
            "ThreadGoalClearResponse",
            "ThreadGoalGetResponse",
            "ThreadGoalSetResponse",
            "ThreadListResponse",
            "ThreadLoadedListResponse",
            "ThreadMetadataUpdateResponse",
            "ThreadReadResponse",
            "ThreadResumeResponse",
            "ThreadRollbackResponse",
            "ThreadStartResponse",
            "ThreadUnarchiveResponse",
            "ThreadUnsubscribeResponse",
            "TurnStartResponse",
            "TurnSteerResponse",
        }};

        std::string_view resultDecoderIdentity(ClientOperationResultDecoder decoder) noexcept {
            const std::size_t index = static_cast<std::size_t>(decoder);
            return index < ResultDecoderIdentities.size() ? ResultDecoderIdentities[index] : std::string_view{};
        }

        ClientOperationDecodeResult
        failure(ClientRequestTarget target, ClientOperationDecodeCode code, ProtocolSurfaceKey surfaceKey, std::string message) {
            return {std::nullopt, {code, target, surfaceKey, "$", std::move(message)}};
        }

        template <typename Result>
        ClientOperationDecodeResult
        decode(ClientRequestTarget target, ProtocolSurfaceKey surfaceKey, std::optional<Result> value, const std::string& decoderError) {
            if (!value) {
                std::string message =
                    decoderError.empty() ? "known successful result does not match its pinned App Server schema" : decoderError;
                return failure(target, ClientOperationDecodeCode::MalformedKnownPayload, surfaceKey, std::move(message));
            }
            return {ClientOperationDecodedValue{std::in_place_type<Result>, std::move(*value)},
                    {ClientOperationDecodeCode::Decoded, target, surfaceKey, "$", "decoded"}};
        }

        const ClientOperationCodecDescriptor* descriptorFor(ClientRequestTarget target) noexcept {
            const std::span<const ClientOperationCodecDescriptor> descriptors = clientOperationCodecDescriptors();
            const ClientOperationCodecDescriptor* result = nullptr;
            for (const ClientOperationCodecDescriptor& candidate : descriptors) {
                if (candidate.target != target) {
                    continue;
                }
                if (result != nullptr) {
                    return nullptr;
                }
                result = &candidate;
            }
            return result;
        }

        bool registryMatches(const ClientOperationCodecDescriptor& descriptor) noexcept {
            try {
                const ProtocolSurfaceEntry& entry = entryFor(descriptor.target);
                const auto* registeredTarget = std::get_if<ClientRequestTarget>(&entry.runtimeTarget);
                return registeredTarget != nullptr && *registeredTarget == descriptor.target && entry.key == descriptor.key &&
                       entry.runtimeDisposition == RuntimeDisposition::Typed &&
                       entry.typedImplementation == TypedImplementationStatus::Implemented &&
                       descriptor.parameterTypeIdentity == entry.operationContract.parameterTypeIdentity &&
                       descriptor.resultTypeIdentity == entry.operationContract.resultTypeIdentity &&
                       descriptor.resultKind == entry.operationContract.resultKind &&
                       descriptor.resultTypeIdentity == resultDecoderIdentity(descriptor.resultDecoder);
            } catch (...) {
                return false;
            }
        }
    } // namespace

    std::string_view clientOperationDecodeCodeName(ClientOperationDecodeCode code) noexcept {
        switch (code) {
            case ClientOperationDecodeCode::Decoded:
                return "Decoded";
            case ClientOperationDecodeCode::MalformedKnownPayload:
                return "MalformedKnownPayload";
            case ClientOperationDecodeCode::MissingContext:
                return "MissingContext";
            case ClientOperationDecodeCode::RegistryDescriptorMismatch:
                return "RegistryDescriptorMismatch";
            case ClientOperationDecodeCode::ResultTypeMismatch:
                return "ResultTypeMismatch";
            case ClientOperationDecodeCode::UnsupportedTarget:
                return "UnsupportedTarget";
        }
        return "UnsupportedTarget";
    }

    std::string_view clientOperationTargetIdentity(ClientRequestTarget target) noexcept {
        const ClientOperationCodecDescriptor* descriptor = descriptorFor(target);
        return descriptor == nullptr ? std::string_view{} : descriptor->runtimeTargetIdentity;
    }

    ClientOperationDecodeResult
    decodeClientOperationResult(ClientRequestTarget target, const Json& raw, std::optional<typed::ThreadId> contextualThreadId) noexcept {
        const ClientOperationCodecDescriptor* descriptor = descriptorFor(target);
        if (target == ClientRequestTarget::Initialize || target == ClientRequestTarget::Count) {
            return failure(
                target, ClientOperationDecodeCode::UnsupportedTarget, {}, "client operation target is outside the A1.1 conversation slice");
        }
        if (descriptor == nullptr || !registryMatches(*descriptor)) {
            return failure(target,
                           ClientOperationDecodeCode::RegistryDescriptorMismatch,
                           descriptor == nullptr ? ProtocolSurfaceKey{} : descriptor->key,
                           "client operation target has no exact canonical registry/descriptor binding");
        }

        try {
            std::string error;
            const ProtocolSurfaceKey key = descriptor->key;
            using enum ClientOperationResultDecoder;
            switch (descriptor->resultDecoder) {
                case Unit:
                    return decode(target, key, decodeUnitResult(raw, error), error);
                case ThreadForkResponse:
                    return decode(target, key, decodeThreadForkResponse(raw, error), error);
                case ThreadGoalClearResponse:
                    return decode(target, key, decodeThreadGoalClearResponse(raw, error), error);
                case ThreadGoalGetResponse:
                    return decode(target, key, decodeThreadGoalGetResponse(raw, error), error);
                case ThreadGoalSetResponse:
                    return decode(target, key, decodeThreadGoalSetResponse(raw, error), error);
                case ThreadListResponse:
                    return decode(target, key, decodeThreadListResponse(raw, error), error);
                case ThreadLoadedListResponse:
                    return decode(target, key, decodeThreadLoadedListResponse(raw, error), error);
                case ThreadMetadataUpdateResponse:
                    return decode(target, key, decodeThreadMetadataUpdateResponse(raw, error), error);
                case ThreadReadResponse:
                    return decode(target, key, decodeThreadReadResponse(raw, error), error);
                case ThreadResumeResponse:
                    return decode(target, key, decodeThreadResumeResponse(raw, error), error);
                case ThreadRollbackResponse:
                    return decode(target, key, decodeThreadRollbackResponse(raw, error), error);
                case ThreadStartResponse:
                    return decode(target, key, decodeThreadStartResponse(raw, error), error);
                case ThreadUnarchiveResponse:
                    return decode(target, key, decodeThreadUnarchiveResponse(raw, error), error);
                case ThreadUnsubscribeResponse:
                    return decode(target, key, decodeThreadUnsubscribeResponse(raw, error), error);
                case TurnStartResponse:
                    if (!contextualThreadId) {
                        return failure(target,
                                       ClientOperationDecodeCode::MissingContext,
                                       key,
                                       "turn/start result decoding requires its submitted ThreadId");
                    }
                    return decode(target, key, decodeTurnStartResponse(raw, *contextualThreadId, error), error);
                case TurnSteerResponse:
                    return decode(target, key, decodeTurnSteerResponse(raw, error), error);
                case Count:
                    break;
            }
        } catch (const std::exception&) {
            return failure(target,
                           ClientOperationDecodeCode::MalformedKnownPayload,
                           descriptor->key,
                           "known successful result decoder raised an exception");
        } catch (...) {
            return failure(target,
                           ClientOperationDecodeCode::MalformedKnownPayload,
                           descriptor->key,
                           "known successful result decoder raised an unknown exception");
        }
        return failure(target, ClientOperationDecodeCode::UnsupportedTarget, descriptor->key, "client operation target is not decodable");
    }

} // namespace ai::openai::codex::detail
