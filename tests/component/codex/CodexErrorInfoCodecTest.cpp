/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/CodexErrorInfoCodec.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/detail/ItemDecoder.h"
#include "ai/openai/codex/detail/ServerRequestDecoder.h"
#include "ai/openai/codex/typed/CodexErrorInfo.h"
#include "ai/openai/codex/typed/Events.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/ServerRequests.h"
#include "support/TestResult.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace {
    using ai::openai::codex::Json;
    using namespace ai::openai::codex::typed;

    constexpr std::string_view FixtureRoot = CODEX_A1_FIXTURE_ROOT;

    Json fixture(std::string_view file) {
        std::ifstream input(std::filesystem::path(FixtureRoot) / "cases/unions/codexerrorinfo" / file);
        return Json::parse(input);
    }

    std::string knownTypeName(const CodexErrorInfo& value) {
        return std::visit(
            [](const auto& alternative) -> std::string {
                using Alternative = std::decay_t<decltype(alternative)>;
                if constexpr (std::is_same_v<Alternative, ActiveTurnNotSteerableCodexErrorInfo>) {
                    return "activeTurnNotSteerable";
                } else if constexpr (std::is_same_v<Alternative, BadRequestCodexErrorInfo>) {
                    return "badRequest";
                } else if constexpr (std::is_same_v<Alternative, ContextWindowExceededCodexErrorInfo>) {
                    return "contextWindowExceeded";
                } else if constexpr (std::is_same_v<Alternative, CyberPolicyCodexErrorInfo>) {
                    return "cyberPolicy";
                } else if constexpr (std::is_same_v<Alternative, HttpConnectionFailedCodexErrorInfo>) {
                    return "httpConnectionFailed";
                } else if constexpr (std::is_same_v<Alternative, InternalServerErrorCodexErrorInfo>) {
                    return "internalServerError";
                } else if constexpr (std::is_same_v<Alternative, OtherCodexErrorInfo>) {
                    return "other";
                } else if constexpr (std::is_same_v<Alternative, ResponseStreamConnectionFailedCodexErrorInfo>) {
                    return "responseStreamConnectionFailed";
                } else if constexpr (std::is_same_v<Alternative, ResponseStreamDisconnectedCodexErrorInfo>) {
                    return "responseStreamDisconnected";
                } else if constexpr (std::is_same_v<Alternative, ResponseTooManyFailedAttemptsCodexErrorInfo>) {
                    return "responseTooManyFailedAttempts";
                } else if constexpr (std::is_same_v<Alternative, SandboxErrorCodexErrorInfo>) {
                    return "sandboxError";
                } else if constexpr (std::is_same_v<Alternative, ServerOverloadedCodexErrorInfo>) {
                    return "serverOverloaded";
                } else if constexpr (std::is_same_v<Alternative, SessionBudgetExceededCodexErrorInfo>) {
                    return "sessionBudgetExceeded";
                } else if constexpr (std::is_same_v<Alternative, ThreadRollbackFailedCodexErrorInfo>) {
                    return "threadRollbackFailed";
                } else if constexpr (std::is_same_v<Alternative, UnauthorizedCodexErrorInfo>) {
                    return "unauthorized";
                } else if constexpr (std::is_same_v<Alternative, UsageLimitExceededCodexErrorInfo>) {
                    return "usageLimitExceeded";
                } else {
                    return "unknown";
                }
            },
            value);
    }

    const Json& retainedRaw(const CodexErrorInfo& value) {
        return std::visit([](const auto& alternative) -> const Json& { return alternative.raw; }, value);
    }

    bool isForwardCompatible(const std::optional<DecodeDiagnostic>& diagnostic, DecodeIssueKind kind) {
        return diagnostic && diagnostic->kind == kind &&
               diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility;
    }

    bool isProtocolWarning(const std::optional<DecodeDiagnostic>& diagnostic) {
        return diagnostic && diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
               diagnostic->severity == DecodeIssueSeverity::ProtocolWarning;
    }

    bool omitsSensitiveValue(const DecodeDiagnostic& diagnostic, std::string_view sensitive) {
        return diagnostic.surface.find(sensitive) == std::string::npos &&
               diagnostic.fieldPath.find(sensitive) == std::string::npos &&
               diagnostic.message.find(sensitive) == std::string::npos;
    }

    template <typename T>
    void checkHttpTriState(tests::support::TestResult& result, std::string_view stem) {
        const Json withValue = fixture(std::string(stem) + ".json");
        const auto decodedValue = ai::openai::codex::detail::decodeCodexErrorInfo(withValue);
        const T* value = decodedValue.value ? std::get_if<T>(&*decodedValue.value) : nullptr;
        result.expectTrue(value && value->httpStatusCode.present && value->httpStatusCode.value == 0 &&
                              value->raw == withValue && !decodedValue.diagnostic,
                          std::string(stem) + " preserves the generated integer status and raw union");

        const Json withNull = fixture(std::string(stem) + "-http-status-null.json");
        const auto decodedNull = ai::openai::codex::detail::decodeCodexErrorInfo(withNull);
        const T* nullValue = decodedNull.value ? std::get_if<T>(&*decodedNull.value) : nullptr;
        result.expectTrue(nullValue && nullValue->httpStatusCode.present && !nullValue->httpStatusCode.value &&
                              nullValue->raw == withNull && !decodedNull.diagnostic,
                          std::string(stem) + " distinguishes an explicit null status");

        const Json omitted = fixture(std::string(stem) + "-http-status-omitted.json");
        const auto decodedOmitted = ai::openai::codex::detail::decodeCodexErrorInfo(omitted);
        const T* omittedValue = decodedOmitted.value ? std::get_if<T>(&*decodedOmitted.value) : nullptr;
        result.expectTrue(omittedValue && !omittedValue->httpStatusCode.present && !omittedValue->httpStatusCode.value &&
                              omittedValue->raw == omitted && !decodedOmitted.diagnostic,
                          std::string(stem) + " distinguishes an omitted status");
    }

} // namespace

int main() {
    tests::support::TestResult result;

    static_assert(std::is_same_v<Item, ThreadItem>);
    static_assert(!std::is_same_v<ThreadItem, ResponseItem>);

    constexpr std::array knownFixtures{
        std::pair{"activeturnnotsteerable.json", "activeTurnNotSteerable"},
        std::pair{"badrequest.json", "badRequest"},
        std::pair{"contextwindowexceeded.json", "contextWindowExceeded"},
        std::pair{"cyberpolicy.json", "cyberPolicy"},
        std::pair{"httpconnectionfailed.json", "httpConnectionFailed"},
        std::pair{"internalservererror.json", "internalServerError"},
        std::pair{"other.json", "other"},
        std::pair{"responsestreamconnectionfailed.json", "responseStreamConnectionFailed"},
        std::pair{"responsestreamdisconnected.json", "responseStreamDisconnected"},
        std::pair{"responsetoomanyfailedattempts.json", "responseTooManyFailedAttempts"},
        std::pair{"sandboxerror.json", "sandboxError"},
        std::pair{"serveroverloaded.json", "serverOverloaded"},
        std::pair{"sessionbudgetexceeded.json", "sessionBudgetExceeded"},
        std::pair{"threadrollbackfailed.json", "threadRollbackFailed"},
        std::pair{"unauthorized.json", "unauthorized"},
        std::pair{"usagelimitexceeded.json", "usageLimitExceeded"},
    };
    for (const auto& [file, expectedType] : knownFixtures) {
        const Json value = fixture(file);
        const auto decoded = ai::openai::codex::detail::decodeCodexErrorInfo(value);
        result.expectTrue(decoded.value && !decoded.diagnostic && knownTypeName(*decoded.value) == expectedType &&
                              retainedRaw(*decoded.value) == value,
                          std::string("generated known CodexErrorInfo fixture decodes exactly: ") + expectedType);
    }

    checkHttpTriState<HttpConnectionFailedCodexErrorInfo>(result, "httpconnectionfailed");
    checkHttpTriState<ResponseStreamConnectionFailedCodexErrorInfo>(result, "responsestreamconnectionfailed");
    checkHttpTriState<ResponseStreamDisconnectedCodexErrorInfo>(result, "responsestreamdisconnected");
    checkHttpTriState<ResponseTooManyFailedAttemptsCodexErrorInfo>(result, "responsetoomanyfailedattempts");

    const auto review =
        ai::openai::codex::detail::decodeCodexErrorInfo(fixture("activeturnnotsteerable.json"));
    const auto* reviewValue =
        review.value ? std::get_if<ActiveTurnNotSteerableCodexErrorInfo>(&*review.value) : nullptr;
    result.expectTrue(reviewValue && reviewValue->turnKind == NonSteerableTurnKind::review() && !review.diagnostic,
                      "review is a known open NonSteerableTurnKind value");

    const auto compact =
        ai::openai::codex::detail::decodeCodexErrorInfo(fixture("activeturnnotsteerable-turn-kind-compact.json"));
    const auto* compactValue =
        compact.value ? std::get_if<ActiveTurnNotSteerableCodexErrorInfo>(&*compact.value) : nullptr;
    result.expectTrue(compactValue && compactValue->turnKind == NonSteerableTurnKind::compact() && !compact.diagnostic,
                      "compact is a known open NonSteerableTurnKind value");

    const Json futureEnumRaw = fixture("activeturnnotsteerable-future-turn-kind.json");
    const auto futureEnum = ai::openai::codex::detail::decodeCodexErrorInfo(futureEnumRaw);
    const auto* futureEnumValue =
        futureEnum.value ? std::get_if<ActiveTurnNotSteerableCodexErrorInfo>(&*futureEnum.value) : nullptr;
    result.expectTrue(futureEnumValue && futureEnumValue->turnKind.value == "futureTurnKind" &&
                          futureEnumValue->raw == futureEnumRaw &&
                          isForwardCompatible(futureEnum.diagnostic, DecodeIssueKind::UnknownEnumValue) &&
                          futureEnum.diagnostic->surface == "CodexErrorInfo" &&
                          futureEnum.diagnostic->fieldPath == "$.activeTurnNotSteerable.turnKind",
                      "future nested enum values stay typed, raw-retained, and forward-compatible");

    const Json futureRaw = fixture("future-unknown.json");
    const auto future = ai::openai::codex::detail::decodeCodexErrorInfo(futureRaw);
    const auto* futureValue = future.value ? std::get_if<UnknownCodexErrorInfo>(&*future.value) : nullptr;
    result.expectTrue(futureValue && futureValue->discriminator == std::optional<std::string>{"futureCodexErrorInfo"} &&
                          futureValue->raw == futureRaw &&
                          isForwardCompatible(future.diagnostic, DecodeIssueKind::UnknownDiscriminator) &&
                          futureValue->diagnostic == *future.diagnostic,
                      "future CodexErrorInfo discriminators retain raw JSON as a low-severity unknown alternative");

    const auto missingRequired =
        ai::openai::codex::detail::decodeCodexErrorInfo(fixture("malformed-known.json"));
    result.expectTrue(!missingRequired.value && isProtocolWarning(missingRequired.diagnostic) &&
                          missingRequired.diagnostic->fieldPath == "$.activeTurnNotSteerable.turnKind",
                      "known alternatives missing a required nested field are protocol warnings");

    const auto wrongRootType = ai::openai::codex::detail::decodeCodexErrorInfo(Json::array({1, 2}));
    result.expectTrue(!wrongRootType.value && isProtocolWarning(wrongRootType.diagnostic) &&
                          wrongRootType.diagnostic->fieldPath == "$",
                      "a known union presented with the wrong root type is a protocol warning");

    const auto nestedWrongType =
        ai::openai::codex::detail::decodeCodexErrorInfo(fixture("nested-wrong-type.json"));
    result.expectTrue(!nestedWrongType.value && isProtocolWarning(nestedWrongType.diagnostic) &&
                          nestedWrongType.diagnostic->fieldPath == "$.activeTurnNotSteerable.turnKind",
                      "a nested known-union field with the wrong type is a protocol warning");

    constexpr std::string_view SensitiveValue = "secret-token-value";
    const Json sensitiveRaw = {
        {"httpConnectionFailed", {{"httpStatusCode", SensitiveValue}}},
    };
    const auto sensitive = ai::openai::codex::detail::decodeCodexErrorInfo(sensitiveRaw);
    result.expectTrue(!sensitive.value && isProtocolWarning(sensitive.diagnostic) &&
                          omitsSensitiveValue(*sensitive.diagnostic, SensitiveValue) &&
                          sensitiveRaw.dump().find(SensitiveValue) != std::string::npos,
                      "malformed diagnostics contain identity/path only while sensitive raw JSON remains separate");

    const Json turnErrorRaw = {
        {"message", "request failed"},
        {"additionalDetails", nullptr},
        {"codexErrorInfo", fixture("httpconnectionfailed.json")},
    };
    std::optional<DecodeDiagnostic> turnDiagnostic;
    const auto turnError = ai::openai::codex::detail::decodeTurnError(turnErrorRaw, turnDiagnostic);
    const auto* turnInfo = turnError && turnError->codexErrorInfo.value
                               ? std::get_if<HttpConnectionFailedCodexErrorInfo>(&*turnError->codexErrorInfo.value)
                               : nullptr;
    result.expectTrue(turnError && turnError->message == "request failed" && turnError->additionalDetails.present &&
                          !turnError->additionalDetails.value && turnError->codexErrorInfo.present && turnInfo &&
                          turnError->raw == turnErrorRaw && !turnError->codexErrorDiagnostic && !turnDiagnostic,
                      "TurnError exposes all known fields, nullability, nested info, and full raw JSON");

    const Json eventRaw = {
        {"method", "error"},
        {"params",
         {{"threadId", "thread-error"},
          {"turnId", "turn-error"},
          {"error", turnErrorRaw},
          {"willRetry", false}}},
    };
    const ai::openai::codex::Notification notification{
        "error",
        eventRaw.at("params"),
        eventRaw,
    };
    const Event decodedEvent = ai::openai::codex::detail::decodeEvent(notification);
    const auto* typedEvent = std::get_if<TurnErrorEvent>(&decodedEvent);
    result.expectTrue(typedEvent && typedEvent->threadId.value == "thread-error" &&
                          typedEvent->turnId.value == "turn-error" && typedEvent->error == turnErrorRaw &&
                          typedEvent->raw == eventRaw && typedEvent->typedError &&
                          typedEvent->typedError->codexErrorInfo.value,
                      "the production error-notification decoder integrates structured TurnError without changing raw fields");

    const Json malformedInfoTurnError = {
        {"message", "request failed"},
        {"codexErrorInfo", fixture("malformed-known.json")},
    };
    const Json malformedEventRaw = {
        {"method", "error"},
        {"params",
         {{"threadId", "thread-error"},
          {"turnId", "turn-error"},
          {"error", malformedInfoTurnError},
          {"willRetry", true}}},
    };
    const Event malformedInfoEvent = ai::openai::codex::detail::decodeEvent(
        {"error", malformedEventRaw.at("params"), malformedEventRaw});
    const auto* malformedTypedEvent = std::get_if<TurnErrorEvent>(&malformedInfoEvent);
    result.expectTrue(malformedTypedEvent && malformedTypedEvent->typedError &&
                          !malformedTypedEvent->typedError->codexErrorInfo.value &&
                          isProtocolWarning(malformedTypedEvent->typedError->codexErrorDiagnostic) &&
                          malformedTypedEvent->error == malformedInfoTurnError,
                      "malformed known nested info remains a nonfatal typed error event with raw preservation");

    const ai::openai::codex::ProtocolError protocolError{
        -32'001,
        "typed remote error",
        std::optional<Json>{turnErrorRaw},
    };
    std::optional<CodexErrorInfo> remoteInfo;
    std::optional<DecodeDiagnostic> remoteDiagnostic;
    ai::openai::codex::detail::decodeProtocolErrorTurnInfo(protocolError, remoteInfo, remoteDiagnostic);
    const auto* remoteHttp = remoteInfo ? std::get_if<HttpConnectionFailedCodexErrorInfo>(&*remoteInfo) : nullptr;
    result.expectTrue(protocolError.code == -32'001 && protocolError.message == "typed remote error" &&
                          protocolError.data == std::optional<Json>{turnErrorRaw} && remoteHttp &&
                          remoteHttp->raw == fixture("httpconnectionfailed.json") && !remoteDiagnostic,
                      "ProtocolError.data decoding adds structured info without modifying raw error code/message/data");

    const Json futureEventRaw = {
        {"method", "future/event"},
        {"params", {{"retained", true}}},
    };
    const Event futureEvent =
        ai::openai::codex::detail::decodeEvent({"future/event", futureEventRaw.at("params"), futureEventRaw});
    const auto* unknownEvent = std::get_if<UnknownEvent>(&futureEvent);
    result.expectTrue(unknownEvent && unknownEvent->raw == futureEventRaw &&
                          isForwardCompatible(unknownEvent->diagnostic, DecodeIssueKind::UnknownMethod) &&
                          unknownEvent->diagnostic->surface == "future/event",
                      "future notification methods retain raw JSON with ForwardCompatibility classification");

    const Json malformedKnownEventRaw = {
        {"method", "thread/started"},
        {"params", Json::array()},
    };
    const Event malformedKnownEvent = ai::openai::codex::detail::decodeEvent(
        {"thread/started", malformedKnownEventRaw.at("params"), malformedKnownEventRaw});
    const auto* malformedEvent = std::get_if<UnknownEvent>(&malformedKnownEvent);
    result.expectTrue(malformedEvent && malformedEvent->raw == malformedKnownEventRaw &&
                          isProtocolWarning(malformedEvent->diagnostic) &&
                          malformedEvent->diagnostic->surface == "thread/started",
                      "known malformed notification payloads retain raw JSON with ProtocolWarning classification");

    const Json futureItemRaw = {
        {"id", "item-future"},
        {"type", "futureItem"},
        {"retained", true},
    };
    std::string itemError;
    const auto futureItem = ai::openai::codex::detail::decodeItem(
        futureItemRaw, ThreadId{"thread-item"}, TurnId{"turn-item"}, itemError);
    const auto* unknownItem = futureItem ? std::get_if<UnknownItem>(&*futureItem) : nullptr;
    result.expectTrue(unknownItem && itemError.empty() && unknownItem->raw == futureItemRaw &&
                          isForwardCompatible(unknownItem->diagnostic, DecodeIssueKind::UnknownDiscriminator),
                      "future item discriminators retain raw JSON with ForwardCompatibility classification");

    const Json malformedItemRaw = {
        {"id", "item-malformed"},
        {"type", "agentMessage"},
    };
    const auto malformedItem = ai::openai::codex::detail::decodeItem(
        malformedItemRaw, ThreadId{"thread-item"}, TurnId{"turn-item"}, itemError);
    const auto* malformedUnknownItem = malformedItem ? std::get_if<UnknownItem>(&*malformedItem) : nullptr;
    result.expectTrue(malformedUnknownItem && itemError.empty() && malformedUnknownItem->raw == malformedItemRaw &&
                          isProtocolWarning(malformedUnknownItem->diagnostic),
                      "known malformed item payloads retain raw JSON with ProtocolWarning classification");

    const Json futureRequestRaw = {
        {"id", "request-future"},
        {"method", "future/request"},
        {"params", {{"retained", true}}},
    };
    const ai::openai::codex::ServerRequest futureServerRequest{
        ai::openai::codex::ServerRequestId{std::string{"request-future"}},
        "future/request",
        futureRequestRaw.at("params"),
        futureRequestRaw,
        ai::openai::codex::ServerRequestToken{7},
    };
    const auto futureRequest =
        ai::openai::codex::detail::decodeServerRequest(futureServerRequest);
    const auto* unknownRequest = std::get_if<UnknownServerRequest>(&futureRequest);
    result.expectTrue(unknownRequest && unknownRequest->raw == futureRequestRaw &&
                          isForwardCompatible(unknownRequest->diagnostic, DecodeIssueKind::UnknownMethod) &&
                          unknownRequest->diagnostic->surface == "future/request",
                      "future server-request methods retain raw JSON with ForwardCompatibility classification");

    const Json malformedRequestRaw = {
        {"id", "request-malformed"},
        {"method", "item/commandExecution/requestApproval"},
        {"params", Json::array()},
    };
    const ai::openai::codex::ServerRequest malformedServerRequest{
        ai::openai::codex::ServerRequestId{std::string{"request-malformed"}},
        "item/commandExecution/requestApproval",
        malformedRequestRaw.at("params"),
        malformedRequestRaw,
        ai::openai::codex::ServerRequestToken{8},
    };
    const auto malformedRequest =
        ai::openai::codex::detail::decodeServerRequest(malformedServerRequest);
    const auto* malformedUnknownRequest = std::get_if<UnknownServerRequest>(&malformedRequest);
    result.expectTrue(malformedUnknownRequest && malformedUnknownRequest->raw == malformedRequestRaw &&
                          isProtocolWarning(malformedUnknownRequest->diagnostic) &&
                          malformedUnknownRequest->diagnostic->surface == "item/commandExecution/requestApproval",
                      "known malformed server requests retain raw JSON with ProtocolWarning classification");

    OperationResult<Json> connectionIndependent;
    connectionIndependent.kind = OperationResult<Json>::Kind::RemoteError;
    connectionIndependent.remoteError = protocolError;
    connectionIndependent.codexErrorInfo = remoteInfo;
    connectionIndependent.codexErrorDiagnostic = remoteDiagnostic;
    result.expectTrue(connectionIndependent.kind == OperationResult<Json>::Kind::RemoteError &&
                          connectionIndependent.remoteError && connectionIndependent.codexErrorInfo &&
                          !connectionIndependent.codexErrorDiagnostic,
                      "structured decoding augments OperationResult without converting the remote failure classification");

    return result.processResult();
}
