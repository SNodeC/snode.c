/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/backend/BackendState.h"
#include "ai/openai/codex/detail/ReviewCodec.h"
#include "ai/openai/codex/typed/Reviews.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    template <typename T>
    concept HasGuardianReviewState = requires(T value) { value.guardianReviews; };

    template <typename T>
    concept HasReviewResultState = requires(T value) { value.reviewResults; };

    template <typename T>
    concept HasGuardianDecisionPolicy = requires(T value) { value.guardianDecisionPolicy; };

    template <typename T>
    concept HasApprovalQueue = requires(T value) { value.approvalQueue; };

    static_assert(!HasGuardianReviewState<backend::BackendState>);
    static_assert(!HasReviewResultState<backend::BackendState>);
    static_assert(!HasGuardianDecisionPolicy<backend::BackendState>);
    static_assert(!HasApprovalQueue<backend::BackendState>);

    bool diagnosticIs(const typed::DecodeDiagnostic& diagnostic,
                      typed::DecodeIssueKind kind,
                      typed::DecodeIssueSeverity severity,
                      std::string_view surface,
                      std::string_view fieldPath) {
        return diagnostic.kind == kind && diagnostic.severity == severity && diagnostic.surface == surface &&
               diagnostic.fieldPath == fieldPath;
    }

    bool hasDiagnostic(const std::vector<typed::DecodeDiagnostic>& diagnostics,
                       typed::DecodeIssueKind kind,
                       typed::DecodeIssueSeverity severity,
                       std::string_view surface,
                       std::string_view fieldPath) {
        for (const typed::DecodeDiagnostic& diagnostic : diagnostics) {
            if (diagnosticIs(diagnostic, kind, severity, surface, fieldPath)) {
                return true;
            }
        }
        return false;
    }

    codex::Notification notification(std::string method, codex::Json params) {
        codex::Json raw{
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params},
            {"futureEnvelopeField", "preserved"},
        };
        return {std::move(method), std::move(params), std::move(raw)};
    }

    codex::Json validTurn(std::string id = "turn-review", std::string status = "inProgress") {
        return {
            {"id", std::move(id)},
            {"items", codex::Json::array()},
            {"status", std::move(status)},
            {"futureTurnField", true},
        };
    }

    codex::Json fullReview(std::string status = "approved") {
        return {
            {"rationale", "synthetic rationale"},
            {"riskLevel", "high"},
            {"status", std::move(status)},
            {"userAuthorization", "medium"},
            {"futureReviewField", true},
        };
    }

    codex::Json commonNotificationParams(codex::Json action) {
        return {
            {"action", std::move(action)},
            {"review", fullReview()},
            {"reviewId", "review-id"},
            {"startedAtMs", std::numeric_limits<std::int64_t>::min()},
            {"targetItemId", nullptr},
            {"threadId", "thread-review"},
            {"turnId", "turn-review"},
        };
    }

    std::vector<codex::Json> guardianActions() {
        return {
            {
                {"command", "synthetic command with spaces"},
                {"cwd", "/synthetic/command cwd"},
                {"source", "shell"},
                {"type", "command"},
            },
            {
                {"argv", codex::Json::array({"synthetic-program", "argument with spaces", ""})},
                {"cwd", "/synthetic/execve cwd"},
                {"program", "synthetic-program"},
                {"source", "unifiedExec"},
                {"type", "execve"},
            },
            {
                {"cwd", "/synthetic/patch cwd"},
                {"files", codex::Json::array({"/synthetic/a.cpp", "/synthetic/path with spaces.cpp"})},
                {"type", "applyPatch"},
            },
            {
                {"host", "synthetic.invalid"},
                {"port", std::numeric_limits<std::uint16_t>::max()},
                {"protocol", "https"},
                {"target", "https://synthetic.invalid/resource"},
                {"type", "networkAccess"},
            },
            {
                {"connectorId", nullptr},
                {"connectorName", "synthetic connector"},
                {"server", "synthetic-mcp"},
                {"toolName", "synthetic_tool"},
                {"toolTitle", nullptr},
                {"type", "mcpToolCall"},
            },
            {
                {"permissions", {{"fileSystem", nullptr}, {"network", {{"enabled", true}}}}},
                {"reason", nullptr},
                {"type", "requestPermissions"},
            },
        };
    }

    void testReviewDelivery(tests::support::TestResult& result) {
        result.expectTrue(typed::ReviewDelivery::inlineReview().value == "inline" && typed::ReviewDelivery::inlineReview().isKnown() &&
                              typed::ReviewDelivery::detached().value == "detached" && typed::ReviewDelivery::detached().isKnown(),
                          "ReviewDelivery exposes both pinned values");
        result.expectTrue(!typed::ReviewDelivery{"futureDelivery"}.isKnown(), "ReviewDelivery remains open to future stable values");

        const typed::ReviewStartParams omitted{
            .threadId = {"thread-review"},
            .target = typed::UncommittedChangesReviewTarget{},
            .delivery = typed::OptionalNullable<typed::ReviewDelivery>::omitted(),
        };
        std::string error = "stale";
        result.expectTrue(detail::encodeReviewStartParams(omitted, error) ==
                                  codex::Json{
                                      {"target", {{"type", "uncommittedChanges"}}},
                                      {"threadId", "thread-review"},
                                  } &&
                              error.empty(),
                          "review/start minimum request omits delivery");

        typed::ReviewStartParams explicitNull = omitted;
        explicitNull.delivery = typed::OptionalNullable<typed::ReviewDelivery>::explicitNull();
        result.expectTrue(detail::encodeReviewStartParams(explicitNull, error) ==
                              codex::Json{
                                  {"delivery", nullptr},
                                  {"target", {{"type", "uncommittedChanges"}}},
                                  {"threadId", "thread-review"},
                              },
                          "review/start preserves explicit-null delivery");

        typed::ReviewStartParams inlineReview = omitted;
        inlineReview.delivery = typed::OptionalNullable<typed::ReviewDelivery>::withValue(typed::ReviewDelivery::inlineReview());
        result.expectTrue(detail::encodeReviewStartParams(inlineReview, error) ==
                              codex::Json{
                                  {"delivery", "inline"},
                                  {"target", {{"type", "uncommittedChanges"}}},
                                  {"threadId", "thread-review"},
                              },
                          "review/start encodes inline delivery exactly");

        typed::ReviewStartParams detached = omitted;
        detached.delivery = typed::OptionalNullable<typed::ReviewDelivery>::withValue(typed::ReviewDelivery::detached());
        result.expectTrue(detail::encodeReviewStartParams(detached, error) ==
                              codex::Json{
                                  {"delivery", "detached"},
                                  {"target", {{"type", "uncommittedChanges"}}},
                                  {"threadId", "thread-review"},
                              },
                          "review/start encodes detached delivery exactly");

        typed::ReviewStartParams future = omitted;
        future.delivery = typed::OptionalNullable<typed::ReviewDelivery>::withValue(typed::ReviewDelivery{"futureDelivery"});
        result.expectTrue(detail::encodeReviewStartParams(future, error) ==
                              codex::Json{
                                  {"delivery", "futureDelivery"},
                                  {"target", {{"type", "uncommittedChanges"}}},
                                  {"threadId", "thread-review"},
                              },
                          "review/start faithfully emits a caller-provided future delivery");

        typed::ReviewStartParams inconsistent = omitted;
        inconsistent.delivery.value = typed::ReviewDelivery{"SYNTHETIC_SECRET_DELIVERY"};
        const auto rejected = detail::encodeReviewStartParams(inconsistent, error);
        result.expectTrue(!rejected && error.find("$.delivery") != std::string::npos &&
                              error.find("SYNTHETIC_SECRET_DELIVERY") == std::string::npos,
                          "review/start rejects an inconsistent tri-state without disclosing its value");
    }

    void testReviewTargets(tests::support::TestResult& result) {
        std::string error = "stale";
        const std::vector<std::pair<typed::ReviewTarget, codex::Json>> cases{
            {
                typed::UncommittedChangesReviewTarget{},
                {{"type", "uncommittedChanges"}},
            },
            {
                typed::BaseBranchReviewTarget{"synthetic/base"},
                {{"branch", "synthetic/base"}, {"type", "baseBranch"}},
            },
            {
                typed::CommitReviewTarget{
                    "0123456789abcdef",
                    typed::OptionalNullable<std::string>::omitted(),
                },
                {{"sha", "0123456789abcdef"}, {"type", "commit"}},
            },
            {
                typed::CustomReviewTarget{"Review only synthetic changes."},
                {{"instructions", "Review only synthetic changes."}, {"type", "custom"}},
            },
        };

        std::size_t exactRoundTrips = 0;
        for (const auto& [value, wire] : cases) {
            const auto encoded = detail::encodeReviewTarget(value, error);
            const auto decoded = detail::decodeReviewTarget(wire);
            exactRoundTrips += encoded && *encoded == wire && error.empty() && decoded.value && decoded.diagnostic == std::nullopt &&
                                       decoded.value->index() == value.index()
                                   ? 1U
                                   : 0U;
        }
        result.expectTrue(exactRoundTrips == cases.size(), "all four ReviewTarget alternatives encode and decode exactly");

        const typed::ReviewTarget commitNull = typed::CommitReviewTarget{
            "fedcba9876543210",
            typed::OptionalNullable<std::string>::explicitNull(),
        };
        const typed::ReviewTarget commitTitle = typed::CommitReviewTarget{
            "fedcba9876543210",
            typed::OptionalNullable<std::string>::withValue("Synthetic commit title"),
        };
        result.expectTrue(detail::encodeReviewTarget(commitNull, error) ==
                                  codex::Json{
                                      {"sha", "fedcba9876543210"},
                                      {"title", nullptr},
                                      {"type", "commit"},
                                  } &&
                              detail::encodeReviewTarget(commitTitle, error) ==
                                  codex::Json{
                                      {"sha", "fedcba9876543210"},
                                      {"title", "Synthetic commit title"},
                                      {"type", "commit"},
                                  },
                          "commit ReviewTarget distinguishes omitted, null, and valued title");

        const codex::Json futureWire{
            {"futurePayload", {{"kept", true}}},
            {"type", "futureTarget"},
        };
        const auto future = detail::decodeReviewTarget(futureWire);
        const auto* unknown = future.value ? std::get_if<typed::UnknownReviewTarget>(&*future.value) : nullptr;
        result.expectTrue(unknown && unknown->type == std::optional<std::string>{"futureTarget"} && unknown->raw == futureWire &&
                              future.diagnostic &&
                              diagnosticIs(*future.diagnostic,
                                           typed::DecodeIssueKind::UnknownDiscriminator,
                                           typed::DecodeIssueSeverity::ForwardCompatibility,
                                           "ReviewTarget",
                                           "$.type"),
                          "future ReviewTarget preserves discriminator, payload, and "
                          "compatibility diagnostic");

        const codex::Json malformedWire{
            {"branch", 7},
            {"type", "baseBranch"},
        };
        const auto malformed = detail::decodeReviewTarget(malformedWire);
        const auto* malformedUnknown = malformed.value ? std::get_if<typed::UnknownReviewTarget>(&*malformed.value) : nullptr;
        result.expectTrue(malformedUnknown && malformedUnknown->raw == malformedWire &&
                              malformedUnknown->type == std::optional<std::string>{"baseBranch"} && malformed.diagnostic &&
                              diagnosticIs(*malformed.diagnostic,
                                           typed::DecodeIssueKind::MalformedKnownPayload,
                                           typed::DecodeIssueSeverity::ProtocolWarning,
                                           "ReviewTarget",
                                           "$.branch"),
                          "malformed known ReviewTarget remains raw and differs from a future target");

        const typed::ReviewTarget unencodable = typed::UnknownReviewTarget{"futureTarget", futureWire, *future.diagnostic};
        const auto rejected = detail::encodeReviewTarget(unencodable, error);
        result.expectTrue(!rejected && error.find("futurePayload") == std::string::npos && error.find("futureTarget") == std::string::npos,
                          "outgoing unknown ReviewTarget requires raw access without leaking its payload");
    }

    void testReviewResponseUsesCanonicalTurn(tests::support::TestResult& result) {
        const codex::Json wire{
            {"reviewThreadId", "thread-detached-review"},
            {"turn", validTurn("turn-detached-review", "inProgress")},
            {"futureResponseField", true},
        };
        std::string error = "stale";
        const auto decoded = detail::decodeReviewStartResponse(wire, error);
        result.expectTrue(decoded && error.empty() && decoded->reviewThreadId.value == "thread-detached-review" &&
                              decoded->turn.id.value == "turn-detached-review" &&
                              decoded->turn.threadId.value == "thread-detached-review" && decoded->turn.status.value == "inProgress" &&
                              decoded->turn.raw == wire.at("turn") && decoded->raw == wire,
                          "ReviewStartResponse reuses the existing Turn codec with review thread context");

        codex::Json malformed = wire;
        malformed["turn"].erase("status");
        const auto rejected = detail::decodeReviewStartResponse(malformed, error);
        result.expectTrue(!rejected && error.find("$.turn") != std::string::npos && error.find("synthetic") == std::string::npos,
                          "ReviewStartResponse rejects a structurally invalid canonical Turn");

        const auto wrongId = detail::decodeReviewStartResponse({{"reviewThreadId", 7}, {"turn", validTurn()}}, error);
        result.expectTrue(!wrongId && error.find("$.reviewThreadId") != std::string::npos,
                          "ReviewStartResponse rejects a wrong review-thread identity type");
    }

    void testGuardianActions(tests::support::TestResult& result) {
        const std::vector<codex::Json> actions = guardianActions();
        std::vector<std::size_t> expectedIndices{0, 1, 2, 3, 4, 5};
        std::size_t decodedCount = 0;
        for (std::size_t index = 0; index < actions.size(); ++index) {
            const auto decoded = detail::decodeGuardianApprovalReviewAction(actions[index]);
            if (decoded.value && decoded.value->index() == expectedIndices[index] && !decoded.diagnostic) {
                ++decodedCount;
            }
        }
        result.expectTrue(decodedCount == actions.size(), "all six GuardianApprovalReviewAction alternatives decode independently");

        const auto command = detail::decodeGuardianApprovalReviewAction(actions[0]);
        const auto* commandAction = command.value ? std::get_if<typed::CommandGuardianApprovalReviewAction>(&*command.value) : nullptr;
        result.expectTrue(commandAction && commandAction->command == "synthetic command with spaces" &&
                              commandAction->cwd.value == "/synthetic/command cwd" &&
                              commandAction->source == typed::GuardianCommandSource::shell() && commandAction->raw == actions[0],
                          "command guardian action preserves command, cwd, source, and raw");

        const auto execve = detail::decodeGuardianApprovalReviewAction(actions[1]);
        const auto* execveAction = execve.value ? std::get_if<typed::ExecveGuardianApprovalReviewAction>(&*execve.value) : nullptr;
        result.expectTrue(
            execveAction && execveAction->argv == std::vector<std::string>({"synthetic-program", "argument with spaces", ""}) &&
                execveAction->program == "synthetic-program" && execveAction->source == typed::GuardianCommandSource::unifiedExec(),
            "execve guardian action preserves argv ordering, empty elements, program, and "
            "source");

        const auto patch = detail::decodeGuardianApprovalReviewAction(actions[2]);
        const auto* patchAction = patch.value ? std::get_if<typed::ApplyPatchGuardianApprovalReviewAction>(&*patch.value) : nullptr;
        result.expectTrue(patchAction && patchAction->files.size() == 2 && patchAction->files[1].value == "/synthetic/path with spaces.cpp",
                          "applyPatch guardian action preserves file ordering and path bytes");

        const auto network = detail::decodeGuardianApprovalReviewAction(actions[3]);
        const auto* networkAction =
            network.value ? std::get_if<typed::NetworkAccessGuardianApprovalReviewAction>(&*network.value) : nullptr;
        result.expectTrue(networkAction && networkAction->port == std::numeric_limits<std::uint16_t>::max() &&
                              networkAction->protocol == typed::NetworkApprovalProtocol::https(),
                          "networkAccess guardian action enforces and preserves uint16 and protocol");

        const auto mcp = detail::decodeGuardianApprovalReviewAction(actions[4]);
        const auto* mcpAction = mcp.value ? std::get_if<typed::McpToolCallGuardianApprovalReviewAction>(&*mcp.value) : nullptr;
        result.expectTrue(mcpAction && mcpAction->connectorId.isNull() &&
                              mcpAction->connectorName.value == std::optional<std::string>{"synthetic connector"} &&
                              mcpAction->toolTitle.isNull(),
                          "mcpToolCall guardian action preserves optional nullable fields");

        const auto permission = detail::decodeGuardianApprovalReviewAction(actions[5]);
        const auto* permissionAction =
            permission.value ? std::get_if<typed::RequestPermissionsGuardianApprovalReviewAction>(&*permission.value) : nullptr;
        result.expectTrue(permissionAction && permissionAction->permissions.fileSystem.isNull() &&
                              permissionAction->permissions.network.value &&
                              permissionAction->permissions.network.value->enabled.value == std::optional<bool>{true} &&
                              permissionAction->reason.isNull(),
                          "requestPermissions guardian action reuses typed permission profile fields");

        codex::Json futureSource = actions[0];
        futureSource["source"] = "futureSource";
        const auto decodedFutureSource = detail::decodeGuardianApprovalReviewAction(futureSource);
        const auto* openCommand =
            decodedFutureSource.value ? std::get_if<typed::CommandGuardianApprovalReviewAction>(&*decodedFutureSource.value) : nullptr;
        result.expectTrue(openCommand && !openCommand->source.isKnown() && decodedFutureSource.diagnostic &&
                              diagnosticIs(*decodedFutureSource.diagnostic,
                                           typed::DecodeIssueKind::UnknownEnumValue,
                                           typed::DecodeIssueSeverity::ForwardCompatibility,
                                           "GuardianCommandSource",
                                           "$.source"),
                          "known guardian action keeps a future nested enum typed and diagnosed");

        const codex::Json futureAction{
            {"opaque", {{"kept", true}}},
            {"type", "futureGuardianAction"},
        };
        const auto future = detail::decodeGuardianApprovalReviewAction(futureAction);
        const auto* unknown = future.value ? std::get_if<typed::UnknownGuardianApprovalReviewAction>(&*future.value) : nullptr;
        result.expectTrue(unknown && unknown->type == std::optional<std::string>{"futureGuardianAction"} && unknown->raw == futureAction &&
                              future.diagnostic &&
                              diagnosticIs(*future.diagnostic,
                                           typed::DecodeIssueKind::UnknownDiscriminator,
                                           typed::DecodeIssueSeverity::ForwardCompatibility,
                                           "GuardianApprovalReviewAction",
                                           "$.type"),
                          "future guardian action preserves its discriminator and raw payload");

        codex::Json malformed = actions[1];
        malformed["argv"] = codex::Json::array({"synthetic-program", 7});
        const auto malformedKnown = detail::decodeGuardianApprovalReviewAction(malformed);
        const auto* malformedUnknown =
            malformedKnown.value ? std::get_if<typed::UnknownGuardianApprovalReviewAction>(&*malformedKnown.value) : nullptr;
        result.expectTrue(malformedUnknown && malformedUnknown->type == std::optional<std::string>{"execve"} &&
                              malformedUnknown->raw == malformed && malformedKnown.diagnostic &&
                              diagnosticIs(*malformedKnown.diagnostic,
                                           typed::DecodeIssueKind::MalformedKnownPayload,
                                           typed::DecodeIssueSeverity::ProtocolWarning,
                                           "GuardianApprovalReviewAction",
                                           "$.argv"),
                          "malformed known guardian action differs from a future action");

        codex::Json overflow = actions[3];
        overflow["port"] = static_cast<std::uint64_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;
        const auto overflowResult = detail::decodeGuardianApprovalReviewAction(overflow);
        const auto* overflowUnknown =
            overflowResult.value ? std::get_if<typed::UnknownGuardianApprovalReviewAction>(&*overflowResult.value) : nullptr;
        result.expectTrue(overflowUnknown && overflowResult.diagnostic && overflowResult.diagnostic->fieldPath == "$.port",
                          "networkAccess rejects a port above uint16 without coercion");
    }

    void testGuardianNotifications(tests::support::TestResult& result) {
        std::string error = "stale";
        const codex::Notification warning = notification("guardianWarning",
                                                         {
                                                             {"message", "Synthetic guardian warning."},
                                                             {"threadId", "thread-warning"},
                                                         });
        const auto decodedWarning = detail::decodeGuardianWarningNotification(warning, error);
        result.expectTrue(decodedWarning && error.empty() && decodedWarning->message == "Synthetic guardian warning." &&
                              decodedWarning->threadId.value == "thread-warning" && decodedWarning->raw == warning.raw,
                          "guardianWarning decodes both stable fields and retains its envelope");

        const auto wrongWarning = detail::decodeGuardianWarningNotification(
            notification("guardianWarning", {{"message", 7}, {"threadId", "SYNTHETIC_SENSITIVE_PATH"}}), error);
        result.expectTrue(!wrongWarning && error.find("$.params.message") != std::string::npos &&
                              error.find("SYNTHETIC_SENSITIVE_PATH") == std::string::npos,
                          "guardianWarning reports structure without leaking payload values");

        const std::vector<codex::Json> actions = guardianActions();
        std::size_t typedStarted = 0;
        for (std::size_t index = 0; index < actions.size(); ++index) {
            codex::Json params = commonNotificationParams(actions[index]);
            if (index == 0) {
                params.erase("targetItemId");
                params["review"] = {{"status", "inProgress"}};
            } else if (index == 1) {
                params["targetItemId"] = "item-review";
            }
            const codex::Notification wire = notification("item/autoApprovalReview/started", std::move(params));
            const auto decoded = detail::decodeItemGuardianApprovalReviewStartedNotification(wire, error);
            if (decoded && decoded->action.index() == index && decoded->raw == wire.raw && error.empty()) {
                ++typedStarted;
            }
            if (index == 0) {
                result.expectTrue(decoded && decoded->targetItemId.isOmitted() && decoded->review.rationale.isOmitted() &&
                                      decoded->review.riskLevel.isOmitted() && decoded->review.userAuthorization.isOmitted(),
                                  "started notification preserves omitted nullable fields");
            } else if (index == 1) {
                result.expectTrue(decoded && decoded->targetItemId.value == std::optional<typed::ItemId>{{"item-review"}},
                                  "started notification preserves a valued target item");
            }
        }
        result.expectTrue(typedStarted == actions.size(), "started notifications dispatch every guardian action alternative");

        codex::Json completedParams = commonNotificationParams(actions[3]);
        completedParams["completedAtMs"] = std::numeric_limits<std::int64_t>::max();
        completedParams["decisionSource"] = "agent";
        const codex::Notification completedWire = notification("item/autoApprovalReview/completed", completedParams);
        const auto completed = detail::decodeItemGuardianApprovalReviewCompletedNotification(completedWire, error);
        result.expectTrue(completed && error.empty() && completed->completedAtMs == std::numeric_limits<std::int64_t>::max() &&
                              completed->startedAtMs == std::numeric_limits<std::int64_t>::min() &&
                              completed->decisionSource == typed::AutoReviewDecisionSource::agent() &&
                              completed->review.status == typed::GuardianApprovalReviewStatus::approved() &&
                              completed->review.rationale.value == std::optional<std::string>{"synthetic rationale"} &&
                              completed->review.riskLevel.value ==
                                  std::optional<typed::GuardianRiskLevel>{typed::GuardianRiskLevel::high()} &&
                              completed->review.userAuthorization.value ==
                                  std::optional<typed::GuardianUserAuthorization>{typed::GuardianUserAuthorization::medium()} &&
                              completed->raw == completedWire.raw,
                          "completed notification preserves timestamps, decision source, review, and raw");

        codex::Json futureCompleted = completedParams;
        futureCompleted["decisionSource"] = "futureDecisionSource";
        futureCompleted["review"]["status"] = "futureReviewStatus";
        futureCompleted["review"]["riskLevel"] = "futureRisk";
        futureCompleted["review"]["userAuthorization"] = "futureAuthorization";
        const auto openCompleted = detail::decodeItemGuardianApprovalReviewCompletedNotification(
            notification("item/autoApprovalReview/completed", futureCompleted), error);
        result.expectTrue(openCompleted && !openCompleted->decisionSource.isKnown() && !openCompleted->review.status.isKnown() &&
                              openCompleted->review.riskLevel.value && !openCompleted->review.riskLevel.value->isKnown() &&
                              openCompleted->review.userAuthorization.value && !openCompleted->review.userAuthorization.value->isKnown() &&
                              hasDiagnostic(openCompleted->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "AutoReviewDecisionSource",
                                            "$.params.decisionSource") &&
                              hasDiagnostic(openCompleted->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "GuardianApprovalReviewStatus",
                                            "$.params.review.status"),
                          "guardian review open enums retain future values with precise diagnostics");

        codex::Json futureParams = commonNotificationParams({{"opaque", {{"kept", true}}}, {"type", "futureGuardianAction"}});
        const auto future = detail::decodeItemGuardianApprovalReviewStartedNotification(
            notification("item/autoApprovalReview/started", futureParams), error);
        const auto* futureAction = future ? std::get_if<typed::UnknownGuardianApprovalReviewAction>(&future->action) : nullptr;
        result.expectTrue(futureAction && futureAction->raw == futureParams.at("action") &&
                              hasDiagnostic(future->diagnostics,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "GuardianApprovalReviewAction",
                                            "$.params.action.type"),
                          "started notification remains typed for a future guardian action");

        codex::Json malformedParams =
            commonNotificationParams({{"command", "SYNTHETIC_SENSITIVE_COMMAND"}, {"cwd", 7}, {"source", "shell"}, {"type", "command"}});
        const auto malformed = detail::decodeItemGuardianApprovalReviewStartedNotification(
            notification("item/autoApprovalReview/started", malformedParams), error);
        const auto* malformedAction = malformed ? std::get_if<typed::UnknownGuardianApprovalReviewAction>(&malformed->action) : nullptr;
        result.expectTrue(malformedAction && hasDiagnostic(malformed->diagnostics,
                                                           typed::DecodeIssueKind::MalformedKnownPayload,
                                                           typed::DecodeIssueSeverity::ProtocolWarning,
                                                           "GuardianApprovalReviewAction",
                                                           "$.params.action.cwd"),
                          "malformed known guardian action remains observable inside a typed notification");

        codex::Json missing = completedParams;
        missing.erase("completedAtMs");
        const auto rejected = detail::decodeItemGuardianApprovalReviewCompletedNotification(
            notification("item/autoApprovalReview/completed", missing), error);
        result.expectTrue(!rejected && error.find("$.params.completedAtMs") != std::string::npos &&
                              error.find("synthetic rationale") == std::string::npos &&
                              error.find("synthetic.invalid") == std::string::npos,
                          "malformed completed notification reports only its structural field path");
    }

    void testGuardianDeniedActionEncoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const std::vector<codex::Json> opaqueEvents{
            nullptr,
            7,
            codex::Json::array({true, nullptr, "synthetic"}),
            {
                {"assessment", "synthetic-denied"},
                {"nested", codex::Json::array({true, nullptr, 7})},
            },
        };
        std::size_t exact = 0;
        for (const codex::Json& event : opaqueEvents) {
            const typed::ThreadApproveGuardianDeniedActionParams params{
                .threadId = {"thread-guardian"},
                .event = event,
            };
            const auto encoded = detail::encodeThreadApproveGuardianDeniedActionParams(params, error);
            exact += encoded && error.empty() && *encoded == codex::Json{{"event", event}, {"threadId", "thread-guardian"}} ? 1U : 0U;
        }
        result.expectTrue(exact == opaqueEvents.size(),
                          "thread guardian approval preserves null, scalar, array, and object "
                          "opaque events exactly");
    }

    void testNoSemanticBackendExpansion(tests::support::TestResult& result) {
        result.expectTrue(!HasGuardianReviewState<backend::BackendState> && !HasReviewResultState<backend::BackendState> &&
                              !HasGuardianDecisionPolicy<backend::BackendState> && !HasApprovalQueue<backend::BackendState>,
                          "A1.3 adds no guardian, review-result, policy, or approval-queue BackendState");
    }
} // namespace

int main() {
    tests::support::TestResult result;
    testReviewDelivery(result);
    testReviewTargets(result);
    testReviewResponseUsesCanonicalTurn(result);
    testGuardianActions(result);
    testGuardianNotifications(result);
    testGuardianDeniedActionEncoding(result);
    testNoSemanticBackendExpansion(result);
    return result.processResult();
}
