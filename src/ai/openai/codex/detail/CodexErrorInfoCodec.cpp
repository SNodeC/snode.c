/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/CodexErrorInfoCodec.h"

#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace ai::openai::codex::detail {

    namespace {

        constexpr const char* Surface = "CodexErrorInfo";

        constexpr CodexErrorInfoCodecDescriptor CodecDescriptors[] = {
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "activeTurnNotSteerable"},
             CodexErrorInfoTarget::ActiveTurnNotSteerable,
             CodexErrorInfoCodecShape::ActiveTurnNotSteerableObject},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "badRequest"},
             CodexErrorInfoTarget::BadRequest,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "contextWindowExceeded"},
             CodexErrorInfoTarget::ContextWindowExceeded,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "cyberPolicy"},
             CodexErrorInfoTarget::CyberPolicy,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "httpConnectionFailed"},
             CodexErrorInfoTarget::HttpConnectionFailed,
             CodexErrorInfoCodecShape::HttpStatusObject},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "internalServerError"},
             CodexErrorInfoTarget::InternalServerError,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "other"},
             CodexErrorInfoTarget::Other,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "responseStreamConnectionFailed"},
             CodexErrorInfoTarget::ResponseStreamConnectionFailed,
             CodexErrorInfoCodecShape::HttpStatusObject},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "responseStreamDisconnected"},
             CodexErrorInfoTarget::ResponseStreamDisconnected,
             CodexErrorInfoCodecShape::HttpStatusObject},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "responseTooManyFailedAttempts"},
             CodexErrorInfoTarget::ResponseTooManyFailedAttempts,
             CodexErrorInfoCodecShape::HttpStatusObject},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "sandboxError"},
             CodexErrorInfoTarget::SandboxError,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "serverOverloaded"},
             CodexErrorInfoTarget::ServerOverloaded,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "sessionBudgetExceeded"},
             CodexErrorInfoTarget::SessionBudgetExceeded,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "threadRollbackFailed"},
             CodexErrorInfoTarget::ThreadRollbackFailed,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "unauthorized"},
             CodexErrorInfoTarget::Unauthorized,
             CodexErrorInfoCodecShape::Scalar},
            {{SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", "usageLimitExceeded"},
             CodexErrorInfoTarget::UsageLimitExceeded,
             CodexErrorInfoCodecShape::Scalar},
        };

        CodexErrorInfoDecodeResult malformed(std::string fieldPath) {
            return {std::nullopt, malformedKnownDiagnostic(Surface, std::move(fieldPath))};
        }

        CodexErrorInfoDecodeResult unknown(const Json& value,
                                           std::optional<std::string> discriminator,
                                           std::string fieldPath) {
            typed::DecodeDiagnostic diagnostic = unknownDiscriminatorDiagnostic(Surface, std::move(fieldPath));
            typed::UnknownCodexErrorInfo unknownValue{std::move(discriminator), value, diagnostic};
            return {typed::CodexErrorInfo{std::move(unknownValue)}, std::move(diagnostic)};
        }

        const CodexErrorInfoCodecDescriptor* descriptorFor(std::string_view discriminator) noexcept {
            const ProtocolSurfaceEntry* entry =
                findSurface(SurfaceCategory::TaggedUnionDiscriminator, Surface, "$variant", discriminator);
            if (entry == nullptr || entry->runtimeDisposition != RuntimeDisposition::Typed ||
                entry->typedImplementation != TypedImplementationStatus::Implemented) {
                return nullptr;
            }
            const CodexErrorInfoTarget* target = std::get_if<CodexErrorInfoTarget>(&entry->runtimeTarget);
            if (target == nullptr) {
                return nullptr;
            }
            const auto descriptors = codexErrorInfoCodecDescriptors();
            const auto descriptor = std::find_if(
                descriptors.begin(), descriptors.end(), [&](const CodexErrorInfoCodecDescriptor& candidate) {
                    return candidate.key == entry->key && candidate.target == *target;
                });
            return descriptor == descriptors.end() ? nullptr : &*descriptor;
        }

        template <typename T>
        CodexErrorInfoDecodeResult scalar(const Json& value) {
            return {typed::CodexErrorInfo{T{value}}, std::nullopt};
        }

        std::optional<typed::OptionalNullable<std::uint16_t>>
        decodeHttpStatus(const Json& nested, std::string_view discriminator, std::optional<typed::DecodeDiagnostic>& diagnostic) {
            typed::OptionalNullable<std::uint16_t> status;
            const auto member = nested.find("httpStatusCode");
            if (member == nested.end()) {
                return status;
            }
            status.present = true;
            if (member->is_null()) {
                return status;
            }

            std::uint64_t decoded = 0;
            if (member->is_number_unsigned()) {
                decoded = member->get<std::uint64_t>();
            } else if (member->is_number_integer()) {
                const std::int64_t signedValue = member->get<std::int64_t>();
                if (signedValue < 0) {
                    diagnostic = malformedKnownDiagnostic(
                        Surface, "$." + std::string(discriminator) + ".httpStatusCode");
                    return std::nullopt;
                }
                decoded = static_cast<std::uint64_t>(signedValue);
            } else {
                diagnostic =
                    malformedKnownDiagnostic(Surface, "$." + std::string(discriminator) + ".httpStatusCode");
                return std::nullopt;
            }
            if (decoded > std::numeric_limits<std::uint16_t>::max()) {
                diagnostic =
                    malformedKnownDiagnostic(Surface, "$." + std::string(discriminator) + ".httpStatusCode");
                return std::nullopt;
            }
            status.value = static_cast<std::uint16_t>(decoded);
            return status;
        }

        template <typename T>
        CodexErrorInfoDecodeResult http(const Json& value, const Json& nested, std::string_view discriminator) {
            std::optional<typed::DecodeDiagnostic> diagnostic;
            auto status = decodeHttpStatus(nested, discriminator, diagnostic);
            if (!status) {
                return {std::nullopt, std::move(diagnostic)};
            }
            return {typed::CodexErrorInfo{T{std::move(*status), value}}, std::nullopt};
        }

        CodexErrorInfoDecodeResult decodeScalar(const Json& value, CodexErrorInfoTarget target) {
            switch (target) {
                case CodexErrorInfoTarget::ContextWindowExceeded:
                    return scalar<typed::ContextWindowExceededCodexErrorInfo>(value);
                case CodexErrorInfoTarget::SessionBudgetExceeded:
                    return scalar<typed::SessionBudgetExceededCodexErrorInfo>(value);
                case CodexErrorInfoTarget::UsageLimitExceeded:
                    return scalar<typed::UsageLimitExceededCodexErrorInfo>(value);
                case CodexErrorInfoTarget::ServerOverloaded:
                    return scalar<typed::ServerOverloadedCodexErrorInfo>(value);
                case CodexErrorInfoTarget::CyberPolicy:
                    return scalar<typed::CyberPolicyCodexErrorInfo>(value);
                case CodexErrorInfoTarget::InternalServerError:
                    return scalar<typed::InternalServerErrorCodexErrorInfo>(value);
                case CodexErrorInfoTarget::Unauthorized:
                    return scalar<typed::UnauthorizedCodexErrorInfo>(value);
                case CodexErrorInfoTarget::BadRequest:
                    return scalar<typed::BadRequestCodexErrorInfo>(value);
                case CodexErrorInfoTarget::ThreadRollbackFailed:
                    return scalar<typed::ThreadRollbackFailedCodexErrorInfo>(value);
                case CodexErrorInfoTarget::SandboxError:
                    return scalar<typed::SandboxErrorCodexErrorInfo>(value);
                case CodexErrorInfoTarget::Other:
                    return scalar<typed::OtherCodexErrorInfo>(value);
                case CodexErrorInfoTarget::HttpConnectionFailed:
                case CodexErrorInfoTarget::ResponseStreamConnectionFailed:
                case CodexErrorInfoTarget::ResponseStreamDisconnected:
                case CodexErrorInfoTarget::ResponseTooManyFailedAttempts:
                case CodexErrorInfoTarget::ActiveTurnNotSteerable:
                case CodexErrorInfoTarget::Count:
                    return malformed("$");
            }
            return malformed("$");
        }

        CodexErrorInfoDecodeResult
        decodeObject(const Json& value,
                     std::string_view discriminator,
                     const CodexErrorInfoCodecDescriptor& descriptor,
                     const Json& nested) {
            if (descriptor.shape == CodexErrorInfoCodecShape::Scalar || !nested.is_object()) {
                return malformed("$." + std::string(discriminator));
            }
            switch (descriptor.target) {
                case CodexErrorInfoTarget::HttpConnectionFailed:
                    return http<typed::HttpConnectionFailedCodexErrorInfo>(value, nested, discriminator);
                case CodexErrorInfoTarget::ResponseStreamConnectionFailed:
                    return http<typed::ResponseStreamConnectionFailedCodexErrorInfo>(value, nested, discriminator);
                case CodexErrorInfoTarget::ResponseStreamDisconnected:
                    return http<typed::ResponseStreamDisconnectedCodexErrorInfo>(value, nested, discriminator);
                case CodexErrorInfoTarget::ResponseTooManyFailedAttempts:
                    return http<typed::ResponseTooManyFailedAttemptsCodexErrorInfo>(value, nested, discriminator);
                case CodexErrorInfoTarget::ActiveTurnNotSteerable: {
                    const auto turnKind = nested.find("turnKind");
                    if (turnKind == nested.end() || !turnKind->is_string()) {
                        return malformed("$.activeTurnNotSteerable.turnKind");
                    }
                    const std::string decodedKind = turnKind->get<std::string>();
                    std::optional<typed::DecodeDiagnostic> diagnostic;
                    if (decodedKind != typed::NonSteerableTurnKind::review().value &&
                        decodedKind != typed::NonSteerableTurnKind::compact().value) {
                        diagnostic = unknownEnumDiagnostic(Surface, "$.activeTurnNotSteerable.turnKind");
                    }
                    return {typed::CodexErrorInfo{typed::ActiveTurnNotSteerableCodexErrorInfo{
                                typed::NonSteerableTurnKind{decodedKind}, value}},
                            std::move(diagnostic)};
                }
                case CodexErrorInfoTarget::ContextWindowExceeded:
                case CodexErrorInfoTarget::SessionBudgetExceeded:
                case CodexErrorInfoTarget::UsageLimitExceeded:
                case CodexErrorInfoTarget::ServerOverloaded:
                case CodexErrorInfoTarget::CyberPolicy:
                case CodexErrorInfoTarget::InternalServerError:
                case CodexErrorInfoTarget::Unauthorized:
                case CodexErrorInfoTarget::BadRequest:
                case CodexErrorInfoTarget::ThreadRollbackFailed:
                case CodexErrorInfoTarget::SandboxError:
                case CodexErrorInfoTarget::Other:
                case CodexErrorInfoTarget::Count:
                    return malformed("$");
            }
            return malformed("$");
        }

    } // namespace

    std::span<const CodexErrorInfoCodecDescriptor> codexErrorInfoCodecDescriptors() noexcept {
        return CodecDescriptors;
    }

    CodexErrorInfoDecodeResult decodeCodexErrorInfo(const Json& value) noexcept {
        try {
            if (value.is_string()) {
                const std::string discriminator = value.get<std::string>();
                const CodexErrorInfoCodecDescriptor* descriptor = descriptorFor(discriminator);
                if (descriptor == nullptr) {
                    return unknown(value, discriminator, "$");
                }
                return descriptor->shape == CodexErrorInfoCodecShape::Scalar
                           ? decodeScalar(value, descriptor->target)
                           : malformed("$");
            }
            if (!value.is_object()) {
                return malformed("$");
            }
            if (value.empty()) {
                return malformed("$");
            }

            std::optional<std::string> knownDiscriminator;
            for (auto member = value.begin(); member != value.end(); ++member) {
                if (descriptorFor(member.key()) != nullptr) {
                    if (knownDiscriminator) {
                        return malformed("$");
                    }
                    knownDiscriminator = member.key();
                }
            }
            if (value.size() != 1) {
                return knownDiscriminator ? malformed("$." + *knownDiscriminator) : malformed("$");
            }

            const auto member = value.begin();
            const std::string discriminator = member.key();
            const CodexErrorInfoCodecDescriptor* descriptor = descriptorFor(discriminator);
            return descriptor == nullptr ? unknown(value, discriminator, "$")
                                         : decodeObject(value, discriminator, *descriptor, member.value());
        } catch (const std::exception&) {
            return malformed("$");
        } catch (...) {
            return malformed("$");
        }
    }

    std::optional<typed::TurnError>
    decodeTurnError(const Json& value, std::optional<typed::DecodeDiagnostic>& diagnostic) noexcept {
        diagnostic.reset();
        try {
            if (!value.is_object()) {
                diagnostic = malformedKnownDiagnostic("TurnError", "$");
                return std::nullopt;
            }
            const auto message = value.find("message");
            if (message == value.end() || !message->is_string()) {
                diagnostic = malformedKnownDiagnostic("TurnError", "$.message");
                return std::nullopt;
            }

            typed::TurnError decoded;
            decoded.message = message->get<std::string>();
            decoded.raw = value;

            const auto additionalDetails = value.find("additionalDetails");
            if (additionalDetails != value.end()) {
                decoded.additionalDetails.present = true;
                if (additionalDetails->is_string()) {
                    decoded.additionalDetails.value = additionalDetails->get<std::string>();
                } else if (!additionalDetails->is_null()) {
                    diagnostic = malformedKnownDiagnostic("TurnError", "$.additionalDetails");
                    return std::nullopt;
                }
            }

            const auto codexErrorInfo = value.find("codexErrorInfo");
            if (codexErrorInfo != value.end()) {
                decoded.codexErrorInfo.present = true;
                if (!codexErrorInfo->is_null()) {
                    CodexErrorInfoDecodeResult info = decodeCodexErrorInfo(*codexErrorInfo);
                    decoded.codexErrorInfo.value = std::move(info.value);
                    decoded.codexErrorDiagnostic = std::move(info.diagnostic);
                }
            }
            return decoded;
        } catch (const std::exception&) {
            diagnostic = malformedKnownDiagnostic("TurnError", "$");
        } catch (...) {
            diagnostic = malformedKnownDiagnostic("TurnError", "$");
        }
        return std::nullopt;
    }

    void decodeProtocolErrorTurnInfo(const ProtocolError& error,
                                     std::optional<typed::CodexErrorInfo>& codexErrorInfo,
                                     std::optional<typed::DecodeDiagnostic>& diagnostic) noexcept {
        codexErrorInfo.reset();
        diagnostic.reset();
        if (!error.data || !error.data->is_object() || error.data->find("codexErrorInfo") == error.data->end()) {
            return;
        }

        std::optional<typed::DecodeDiagnostic> turnDiagnostic;
        std::optional<typed::TurnError> turnError = decodeTurnError(*error.data, turnDiagnostic);
        if (!turnError) {
            diagnostic = std::move(turnDiagnostic);
            return;
        }
        if (turnError->codexErrorInfo.value) {
            codexErrorInfo = std::move(*turnError->codexErrorInfo.value);
        }
        diagnostic = std::move(turnError->codexErrorDiagnostic);
    }

} // namespace ai::openai::codex::detail
