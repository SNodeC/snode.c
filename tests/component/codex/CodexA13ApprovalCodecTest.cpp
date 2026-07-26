/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ApprovalCodec.h"
#include "ai/openai/codex/typed/ServerRequests.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    template <typename Alternative, typename Union>
    bool holds(const detail::ConversationDecodeResult<Union>& decoded) {
        return decoded.value && std::holds_alternative<Alternative>(*decoded.value);
    }

    template <typename Union>
    bool hasDiagnostic(const detail::ConversationDecodeResult<Union>& decoded,
                       typed::DecodeIssueKind kind,
                       typed::DecodeIssueSeverity severity,
                       const std::string& surface,
                       const std::string& path) {
        return decoded.diagnostic && decoded.diagnostic->kind == kind && decoded.diagnostic->severity == severity &&
               decoded.diagnostic->surface == surface && decoded.diagnostic->fieldPath == path;
    }

    void testFileChangeUnion(tests::support::TestResult& result) {
        const auto add = detail::decodeFileChange({{"type", "add"}, {"content", "synthetic-add-content"}});
        const auto remove = detail::decodeFileChange({{"type", "delete"}, {"content", "synthetic-delete-content"}});
        const auto update =
            detail::decodeFileChange({{"type", "update"}, {"move_path", nullptr}, {"unified_diff", "synthetic-unified-diff"}});
        result.expectTrue(holds<typed::AddFileChange>(add) &&
                              std::get<typed::AddFileChange>(*add.value).content == "synthetic-add-content" &&
                              holds<typed::DeleteFileChange>(remove) &&
                              std::get<typed::DeleteFileChange>(*remove.value).content == "synthetic-delete-content" &&
                              holds<typed::UpdateFileChange>(update) && std::get<typed::UpdateFileChange>(*update.value).movePath.isNull(),
                          "FileChange decodes add, delete, and update without inventing rename");

        std::string error = "stale";
        result.expectTrue(detail::encodeFileChange(*add.value, error) ==
                                  codex::Json{{"type", "add"}, {"content", "synthetic-add-content"}} &&
                              detail::encodeFileChange(*remove.value, error) ==
                                  codex::Json{{"type", "delete"}, {"content", "synthetic-delete-content"}} &&
                              detail::encodeFileChange(*update.value, error) ==
                                  codex::Json{{"type", "update"}, {"move_path", nullptr}, {"unified_diff", "synthetic-unified-diff"}} &&
                              error.empty(),
                          "all known FileChange alternatives encode exact stable wire keys");

        const auto future = detail::decodeFileChange({{"type", "future-change"}, {"opaque", 17}});
        const auto malformed = detail::decodeFileChange({{"type", "update"}, {"unified_diff", 17}});
        result.expectTrue(holds<typed::UnrecognizedFileChange>(future) &&
                              hasDiagnostic(future,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "FileChange",
                                            "$.type") &&
                              std::get<typed::UnrecognizedFileChange>(*future.value).raw ==
                                  codex::Json{{"type", "future-change"}, {"opaque", 17}},
                          "future FileChange alternatives preserve raw data and forward compatibility");
        result.expectTrue(holds<typed::UnrecognizedFileChange>(malformed) && hasDiagnostic(malformed,
                                                                                           typed::DecodeIssueKind::MalformedKnownPayload,
                                                                                           typed::DecodeIssueSeverity::ProtocolWarning,
                                                                                           "FileChange",
                                                                                           "$.unified_diff"),
                          "malformed known FileChange alternatives remain distinct from future ones");
    }

    void testFileSystemPathUnions(tests::support::TestResult& result) {
        using SpecialCase = std::pair<codex::Json, bool (*)(const typed::FileSystemSpecialPath&)>;
        const std::vector<SpecialCase> specialCases{
            {{{"kind", "root"}},
             [](const typed::FileSystemSpecialPath& value) {
                 return std::holds_alternative<typed::RootFileSystemSpecialPath>(value);
             }},
            {{{"kind", "minimal"}},
             [](const typed::FileSystemSpecialPath& value) {
                 return std::holds_alternative<typed::MinimalFileSystemSpecialPath>(value);
             }},
            {{{"kind", "project_roots"}, {"subpath", nullptr}},
             [](const typed::FileSystemSpecialPath& value) {
                 const auto* path = std::get_if<typed::ProjectRootsFileSystemSpecialPath>(&value);
                 return path != nullptr && path->subpath.isNull();
             }},
            {{{"kind", "tmpdir"}},
             [](const typed::FileSystemSpecialPath& value) {
                 return std::holds_alternative<typed::TmpdirFileSystemSpecialPath>(value);
             }},
            {{{"kind", "slash_tmp"}},
             [](const typed::FileSystemSpecialPath& value) {
                 return std::holds_alternative<typed::SlashTmpFileSystemSpecialPath>(value);
             }},
            {{{"kind", "unknown"}, {"path", "/synthetic/unknown-path"}, {"subpath", "child"}},
             [](const typed::FileSystemSpecialPath& value) {
                 const auto* path = std::get_if<typed::UnknownFileSystemSpecialPath>(&value);
                 return path != nullptr && path->path == "/synthetic/unknown-path" &&
                        path->subpath.value == std::optional<std::string>{"child"};
             }},
        };

        std::string error;
        std::size_t specialCount = 0;
        for (const auto& [wire, predicate] : specialCases) {
            const auto decoded = detail::decodeFileSystemSpecialPath(wire);
            if (decoded.value && predicate(*decoded.value) && detail::encodeFileSystemSpecialPath(*decoded.value, error) == wire &&
                error.empty()) {
                ++specialCount;
            }
        }
        result.expectEqual(specialCases.size(), specialCount, "all six known FileSystemSpecialPath alternatives round-trip exactly");

        const auto knownLiteralUnknown = detail::decodeFileSystemSpecialPath({{"kind", "unknown"}, {"path", "/synthetic/literal-unknown"}});
        const auto future = detail::decodeFileSystemSpecialPath({{"kind", "future-special"}, {"opaque", true}});
        const auto malformed = detail::decodeFileSystemSpecialPath({{"kind", "unknown"}, {"path", false}});
        result.expectTrue(holds<typed::UnknownFileSystemSpecialPath>(knownLiteralUnknown) && !knownLiteralUnknown.diagnostic,
                          "the pinned FileSystemSpecialPath literal unknown is a known alternative");
        result.expectTrue(holds<typed::UnrecognizedFileSystemSpecialPath>(future) &&
                              hasDiagnostic(future,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "FileSystemSpecialPath",
                                            "$.kind"),
                          "a genuinely future FileSystemSpecialPath kind is forward-compatible");
        result.expectTrue(holds<typed::UnrecognizedFileSystemSpecialPath>(malformed) &&
                              hasDiagnostic(malformed,
                                            typed::DecodeIssueKind::MalformedKnownPayload,
                                            typed::DecodeIssueSeverity::ProtocolWarning,
                                            "FileSystemSpecialPath",
                                            "$.path"),
                          "a malformed pinned unknown special path is a protocol warning");

        const std::vector<codex::Json> pathCases{
            {{"type", "path"}, {"path", "./synthetic/../wire-path"}},
            {{"type", "glob_pattern"}, {"pattern", "**/synthetic-*.cpp"}},
            {{"type", "special"}, {"value", {{"kind", "slash_tmp"}}}},
        };
        std::size_t pathCount = 0;
        for (const codex::Json& wire : pathCases) {
            const auto decoded = detail::decodeFileSystemPath(wire);
            if (decoded.value && detail::encodeFileSystemPath(*decoded.value, error) == wire && error.empty()) {
                ++pathCount;
            }
        }
        result.expectEqual(pathCases.size(), pathCount, "all three FileSystemPath alternatives preserve path bytes");

        const auto futurePath = detail::decodeFileSystemPath({{"type", "future-path"}, {"opaque", 1}});
        const auto malformedPath = detail::decodeFileSystemPath({{"type", "special"}, {"value", 1}});
        const auto* malformedSpecial = malformedPath.value ? std::get_if<typed::SpecialFileSystemPath>(&*malformedPath.value) : nullptr;
        result.expectTrue(holds<typed::UnrecognizedFileSystemPath>(futurePath) &&
                              hasDiagnostic(futurePath,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "FileSystemPath",
                                            "$.type") &&
                              malformedSpecial != nullptr &&
                              std::holds_alternative<typed::UnrecognizedFileSystemSpecialPath>(malformedSpecial->value) &&
                              hasDiagnostic(malformedPath,
                                            typed::DecodeIssueKind::MalformedKnownPayload,
                                            typed::DecodeIssueSeverity::ProtocolWarning,
                                            "FileSystemSpecialPath",
                                            "$.value"),
                          "FileSystemPath distinguishes a future discriminator from malformed special data");
    }

    void testParsedCommandUnion(tests::support::TestResult& result) {
        const std::vector<codex::Json> cases{
            {{"type", "read"}, {"cmd", "synthetic-read"}, {"name", "synthetic-name"}, {"path", "/synthetic/read"}},
            {{"type", "list_files"}, {"cmd", "synthetic-list"}, {"path", nullptr}},
            {{"type", "search"}, {"cmd", "synthetic-search"}, {"path", "/synthetic/search"}, {"query", nullptr}},
            {{"type", "unknown"}, {"cmd", "synthetic-known-unknown"}},
        };
        std::string error;
        std::size_t knownCount = 0;
        for (const codex::Json& wire : cases) {
            const auto decoded = detail::decodeParsedCommand(wire);
            if (decoded.value && detail::encodeParsedCommand(*decoded.value, error) == wire && error.empty()) {
                ++knownCount;
            }
        }
        result.expectEqual(cases.size(), knownCount, "all four known ParsedCommand alternatives round-trip exactly");

        const auto knownLiteralUnknown = detail::decodeParsedCommand({{"type", "unknown"}, {"cmd", "synthetic"}});
        const auto future = detail::decodeParsedCommand({{"type", "future-command"}, {"cmd", "synthetic"}});
        const auto malformed = detail::decodeParsedCommand({{"type", "read"}, {"cmd", "synthetic"}});
        result.expectTrue(holds<typed::UnknownParsedCommand>(knownLiteralUnknown) && !knownLiteralUnknown.diagnostic,
                          "ParsedCommand literal unknown is a pinned known value");
        result.expectTrue(holds<typed::UnrecognizedParsedCommand>(future) && hasDiagnostic(future,
                                                                                           typed::DecodeIssueKind::UnknownDiscriminator,
                                                                                           typed::DecodeIssueSeverity::ForwardCompatibility,
                                                                                           "ParsedCommand",
                                                                                           "$.type"),
                          "future ParsedCommand alternatives preserve forward compatibility");
        result.expectTrue(holds<typed::UnrecognizedParsedCommand>(malformed) && hasDiagnostic(malformed,
                                                                                              typed::DecodeIssueKind::MalformedKnownPayload,
                                                                                              typed::DecodeIssueSeverity::ProtocolWarning,
                                                                                              "ParsedCommand",
                                                                                              "$.name"),
                          "malformed known ParsedCommand alternatives surface a protocol warning");
    }

    void testDecisionUnions(tests::support::TestResult& result) {
        const std::vector<codex::Json> commandCases{
            "accept",
            "acceptForSession",
            {{"acceptWithExecpolicyAmendment", {{"execpolicy_amendment", codex::Json::array({"synthetic-rule-a", "synthetic-rule-b"})}}}},
            {{"applyNetworkPolicyAmendment", {{"network_policy_amendment", {{"action", "allow"}, {"host", "synthetic.invalid"}}}}}},
            "cancel",
            "decline",
        };
        std::string error;
        std::size_t commandCount = 0;
        for (const codex::Json& wire : commandCases) {
            const auto decoded = detail::decodeCommandExecutionApprovalDecision(wire);
            if (decoded.value && detail::encodeCommandExecutionApprovalDecision(*decoded.value, error) == wire && error.empty()) {
                ++commandCount;
            }
        }
        result.expectEqual(commandCases.size(), commandCount, "all six CommandExecutionApprovalDecision alternatives round-trip exactly");

        const auto futureCommand = detail::decodeCommandExecutionApprovalDecision("future-decision");
        const auto malformedCommand =
            detail::decodeCommandExecutionApprovalDecision({{"acceptWithExecpolicyAmendment", {{"execpolicy_amendment", "wrong-type"}}}});
        result.expectTrue(holds<typed::UnrecognizedCommandExecutionApprovalDecision>(futureCommand) &&
                              hasDiagnostic(futureCommand,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "CommandExecutionApprovalDecision",
                                            "$") &&
                              holds<typed::UnrecognizedCommandExecutionApprovalDecision>(malformedCommand) &&
                              hasDiagnostic(malformedCommand,
                                            typed::DecodeIssueKind::MalformedKnownPayload,
                                            typed::DecodeIssueSeverity::ProtocolWarning,
                                            "CommandExecutionApprovalDecision",
                                            "$.acceptWithExecpolicyAmendment.execpolicy_amendment"),
                          "command decisions distinguish future from malformed known alternatives");

        const std::vector<codex::Json> reviewCases{
            "abort",
            "approved",
            {{"approved_execpolicy_amendment", {{"proposed_execpolicy_amendment", codex::Json::array({"synthetic-rule"})}}}},
            "approved_for_session",
            "denied",
            {{"network_policy_amendment", {{"network_policy_amendment", {{"action", "deny"}, {"host", "synthetic.invalid"}}}}}},
            "timed_out",
        };
        std::size_t reviewCount = 0;
        for (const codex::Json& wire : reviewCases) {
            const auto decoded = detail::decodeReviewDecision(wire);
            if (decoded.value && detail::encodeReviewDecision(*decoded.value, error) == wire && error.empty()) {
                ++reviewCount;
            }
        }
        result.expectEqual(reviewCases.size(), reviewCount, "all seven ReviewDecision alternatives round-trip exactly");

        const auto futureReview = detail::decodeReviewDecision("future-review-decision");
        const auto malformedReview = detail::decodeReviewDecision(
            {{"network_policy_amendment", {{"network_policy_amendment", {{"action", false}, {"host", "x"}}}}}});
        result.expectTrue(holds<typed::UnrecognizedReviewDecision>(futureReview) &&
                              hasDiagnostic(futureReview,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "ReviewDecision",
                                            "$") &&
                              holds<typed::UnrecognizedReviewDecision>(malformedReview) &&
                              hasDiagnostic(malformedReview,
                                            typed::DecodeIssueKind::MalformedKnownPayload,
                                            typed::DecodeIssueSeverity::ProtocolWarning,
                                            "ReviewDecision",
                                            "$.network_policy_amendment.network_policy_amendment.action"),
                          "review decisions distinguish future from malformed known alternatives");
    }

    codex::Json requestPermissionProfile() {
        const codex::Json entries = codex::Json::array(
            {{{"access", "read"}, {"path", {{"type", "path"}, {"path", "/synthetic/exact-path"}}}},
             {{"access", "write"}, {"path", {{"type", "glob_pattern"}, {"pattern", "**/synthetic-*"}}}},
             {{"access", "deny"},
              {"path",
               {{"type", "special"}, {"value", {{"kind", "unknown"}, {"path", "/synthetic/unknown-base"}, {"subpath", nullptr}}}}}}});
        return {
            {"fileSystem",
             {{"entries", entries}, {"globScanMaxDepth", 1}, {"read", codex::Json::array({"/synthetic/legacy-read"})}, {"write", nullptr}}},
            {"network", {{"enabled", true}}},
        };
    }

    codex::Json applyPatchParams() {
        return {
            {"callId", "call-patch"},
            {"conversationId", "thread-patch"},
            {"fileChanges",
             {{"/synthetic/add", {{"type", "add"}, {"content", "synthetic-add"}}},
              {"/synthetic/delete", {{"type", "delete"}, {"content", "synthetic-delete"}}},
              {"/synthetic/update", {{"type", "update"}, {"move_path", nullptr}, {"unified_diff", "synthetic-diff"}}}}},
            {"grantRoot", nullptr},
            {"reason", "synthetic patch reason"},
        };
    }

    codex::Json execCommandParams() {
        return {
            {"approvalId", nullptr},
            {"callId", "call-exec"},
            {"command", codex::Json::array({"synthetic-command", "argument with spaces", ""})},
            {"conversationId", "thread-exec"},
            {"cwd", "./synthetic/../exec-cwd"},
            {"parsedCmd",
             codex::Json::array({{{"type", "read"}, {"cmd", "read"}, {"name", "name"}, {"path", "/synthetic/read"}},
                                 {{"type", "list_files"}, {"cmd", "list"}, {"path", nullptr}},
                                 {{"type", "search"}, {"cmd", "search"}, {"path", "/synthetic/search"}, {"query", "needle"}},
                                 {{"type", "unknown"}, {"cmd", "synthetic-unknown"}}})},
            {"reason", nullptr},
        };
    }

    codex::Json commandRequestParams() {
        return {
            {"approvalId", "approval-command"},
            {"command", "synthetic command description"},
            {"commandActions", codex::Json::array({{{"type", "unknown"}, {"command", "synthetic-action"}}})},
            {"cwd", "/synthetic/command-cwd"},
            {"environmentId", nullptr},
            {"itemId", "item-command"},
            {"networkApprovalContext", {{"host", "synthetic.invalid"}, {"protocol", "https"}}},
            {"proposedExecpolicyAmendment", codex::Json::array({"synthetic-policy"})},
            {"proposedNetworkPolicyAmendments", codex::Json::array({{{"action", "allow"}, {"host", "synthetic.invalid"}}})},
            {"reason", "synthetic command reason"},
            {"startedAtMs", std::numeric_limits<std::int64_t>::min()},
            {"threadId", "thread-command"},
            {"turnId", "turn-command"},
        };
    }

    codex::Json fileChangeRequestParams() {
        return {
            {"grantRoot", nullptr},
            {"itemId", "item-file"},
            {"reason", "synthetic file reason"},
            {"startedAtMs", std::numeric_limits<std::int64_t>::max()},
            {"threadId", "thread-file"},
            {"turnId", "turn-file"},
        };
    }

    codex::Json permissionsRequestParams() {
        return {
            {"cwd", "/synthetic/permission-cwd"},
            {"environmentId", nullptr},
            {"itemId", "item-permission"},
            {"permissions", requestPermissionProfile()},
            {"reason", "synthetic permission reason"},
            {"startedAtMs", 0},
            {"threadId", "thread-permission"},
            {"turnId", "turn-permission"},
        };
    }

    void testRequestRoots(tests::support::TestResult& result) {
        std::string error = "stale";
        const auto minimumPatch = detail::decodeApplyPatchApprovalParams(
            {{"callId", "minimum-patch"}, {"conversationId", "minimum-thread"}, {"fileChanges", codex::Json::object()}}, error);
        const auto minimumExec = detail::decodeExecCommandApprovalParams({{"callId", "minimum-exec"},
                                                                          {"command", codex::Json::array()},
                                                                          {"conversationId", "minimum-thread"},
                                                                          {"cwd", ""},
                                                                          {"parsedCmd", codex::Json::array()}},
                                                                         error);
        const codex::Json minimumCommon{
            {"itemId", "minimum-item"}, {"startedAtMs", 0}, {"threadId", "minimum-thread"}, {"turnId", "minimum-turn"}};
        const auto minimumCommand = detail::decodeCommandExecutionRequestApprovalParams(minimumCommon, error);
        const auto minimumFile = detail::decodeFileChangeRequestApprovalParams(minimumCommon, error);
        codex::Json minimumPermissions = minimumCommon;
        minimumPermissions["cwd"] = "/synthetic/minimum";
        minimumPermissions["permissions"] = codex::Json::object();
        const auto minimumPermissionRequest = detail::decodePermissionsRequestApprovalParams(minimumPermissions, error);
        result.expectTrue(minimumPatch && minimumPatch->grantRoot.isOmitted() && minimumPatch->reason.isOmitted() && minimumExec &&
                              minimumExec->approvalId.isOmitted() && minimumExec->reason.isOmitted() && minimumCommand &&
                              minimumCommand->approvalId.isOmitted() && minimumCommand->command.isOmitted() &&
                              minimumCommand->commandActions.isOmitted() && minimumFile && minimumFile->grantRoot.isOmitted() &&
                              minimumFile->reason.isOmitted() && minimumPermissionRequest &&
                              minimumPermissionRequest->environmentId.isOmitted() && minimumPermissionRequest->reason.isOmitted() &&
                              error.empty(),
                          "all five server-request roots decode their minimum valid omitted-field forms");

        const auto patch = detail::decodeApplyPatchApprovalParams(applyPatchParams(), error);
        result.expectTrue(patch && error.empty() && patch->callId.value == "call-patch" && patch->conversationId.value == "thread-patch" &&
                              patch->fileChanges.size() == 3 && patch->grantRoot.isNull() &&
                              patch->reason.value == std::optional<std::string>{"synthetic patch reason"} &&
                              patch->raw == applyPatchParams(),
                          "applyPatchApproval decodes every stable field and all FileChange alternatives");

        const auto exec = detail::decodeExecCommandApprovalParams(execCommandParams(), error);
        result.expectTrue(exec && error.empty() && exec->approvalId.isNull() &&
                              exec->command == std::vector<std::string>({"synthetic-command", "argument with spaces", ""}) &&
                              exec->parsedCommand.size() == 4 && exec->cwd == "./synthetic/../exec-cwd" && exec->reason.isNull() &&
                              exec->raw == execCommandParams(),
                          "execCommandApproval decodes argv, parsed commands, nullability, and raw data");

        const auto command = detail::decodeCommandExecutionRequestApprovalParams(commandRequestParams(), error);
        result.expectTrue(command && error.empty() && command->approvalId.value == std::optional<std::string>{"approval-command"} &&
                              command->cwd.value && command->cwd.value->value == "/synthetic/command-cwd" &&
                              command->environmentId.isNull() && command->networkApprovalContext.value &&
                              command->networkApprovalContext.value->protocol == typed::NetworkApprovalProtocol::https() &&
                              command->proposedExecpolicyAmendment.value && command->proposedNetworkPolicyAmendments.value &&
                              command->startedAtMs == std::numeric_limits<std::int64_t>::min() && command->raw == commandRequestParams(),
                          "item command approval completes every formerly partial stable field");

        const auto file = detail::decodeFileChangeRequestApprovalParams(fileChangeRequestParams(), error);
        result.expectTrue(file && error.empty() && file->grantRoot.isNull() &&
                              file->reason.value == std::optional<std::string>{"synthetic file reason"} &&
                              file->startedAtMs == std::numeric_limits<std::int64_t>::max() && file->raw == fileChangeRequestParams(),
                          "item file-change approval completes its former partial shape");

        const auto permissions = detail::decodePermissionsRequestApprovalParams(permissionsRequestParams(), error);
        result.expectTrue(
            permissions && error.empty() && permissions->cwd.value == "/synthetic/permission-cwd" && permissions->environmentId.isNull() &&
                permissions->permissions.fileSystem.value && permissions->permissions.fileSystem.value->entries.value &&
                permissions->permissions.fileSystem.value->entries.value->size() == 3 && permissions->permissions.network.value &&
                permissions->permissions.network.value->enabled.value == std::optional<bool>{true} &&
                permissions->raw == permissionsRequestParams(),
            "permission approval decodes paths, profiles, nullability, and raw data");

        codex::Json missing = commandRequestParams();
        missing.erase("threadId");
        const auto missingCommand = detail::decodeCommandExecutionRequestApprovalParams(missing, error);
        result.expectTrue(!missingCommand && error.find("$.threadId") != std::string::npos &&
                              error.find("synthetic command description") == std::string::npos,
                          "request-root diagnostics identify structure without leaking command data");

        codex::Json wrong = permissionsRequestParams();
        wrong["permissions"]["fileSystem"]["entries"][2]["path"]["value"]["kind"] = 17;
        const auto wrongPermissions = detail::decodePermissionsRequestApprovalParams(wrong, error);
        bool malformedKnown = false;
        if (wrongPermissions) {
            for (const typed::DecodeDiagnostic& diagnostic : wrongPermissions->diagnostics) {
                malformedKnown = malformedKnown || (diagnostic.kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                                                    diagnostic.severity == typed::DecodeIssueSeverity::ProtocolWarning &&
                                                    diagnostic.fieldPath.find("$.permissions.fileSystem.entries[2].path.value") == 0);
            }
        }
        result.expectTrue(wrongPermissions && malformedKnown && error.empty(),
                          "nested malformed permission unions remain typed with redacted structural diagnostics");
    }

    void testResponseRoots(tests::support::TestResult& result) {
        std::string error;
        const typed::ApplyPatchApprovalResponse patch{typed::ApprovedExecpolicyAmendmentReviewDecision{{"synthetic-execpolicy"}}};
        const typed::ExecCommandApprovalResponse exec{
            typed::NetworkPolicyAmendmentReviewDecision{{.action = typed::NetworkPolicyRuleAction::deny(),
                                                         .host = "synthetic.invalid",
                                                         .raw = codex::Json::object(),
                                                         .diagnostics = {}}}};
        const typed::CommandExecutionRequestApprovalResponse command{
            typed::AcceptWithExecpolicyAmendmentCommandExecutionApprovalDecision{{"synthetic-command-policy"}}};
        const typed::FileChangeRequestApprovalResponse file{typed::FileChangeApprovalDecision::acceptForSession()};

        typed::GrantedPermissionProfile granted;
        typed::AdditionalFileSystemPermissions fileSystem;
        fileSystem.globScanMaxDepth = typed::OptionalNullable<std::uint64_t>::withValue(std::numeric_limits<std::uint64_t>::max());
        fileSystem.entries = typed::OptionalNullable<std::vector<typed::FileSystemSandboxEntry>>::withValue(
            {{.access = typed::FileSystemAccessMode::write(),
              .path = typed::SpecialFileSystemPath{.value = typed::SlashTmpFileSystemSpecialPath{},
                                                   .raw = codex::Json::object(),
                                                   .diagnostics = {}},
              .raw = codex::Json::object(),
              .diagnostics = {}}});
        granted.fileSystem = typed::OptionalNullable<typed::AdditionalFileSystemPermissions>::withValue(std::move(fileSystem));
        granted.network = typed::OptionalNullable<typed::AdditionalNetworkPermissions>::explicitNull();
        const typed::PermissionsRequestApprovalResponse permissions{
            std::move(granted), typed::PermissionGrantScope::session(), typed::OptionalNullable<bool>::withValue(false)};

        result.expectTrue(
            detail::encodeApplyPatchApprovalResponse(patch, error) ==
                    codex::Json{{"decision",
                                 {{"approved_execpolicy_amendment",
                                   {{"proposed_execpolicy_amendment", codex::Json::array({"synthetic-execpolicy"})}}}}}} &&
                detail::encodeExecCommandApprovalResponse(exec, error) ==
                    codex::Json{{"decision",
                                 {{"network_policy_amendment",
                                   {{"network_policy_amendment", {{"action", "deny"}, {"host", "synthetic.invalid"}}}}}}}} &&
                detail::encodeCommandExecutionRequestApprovalResponse(command, error) ==
                    codex::Json{{"decision",
                                 {{"acceptWithExecpolicyAmendment",
                                   {{"execpolicy_amendment", codex::Json::array({"synthetic-command-policy"})}}}}}} &&
                detail::encodeFileChangeRequestApprovalResponse(file, error) == codex::Json{{"decision", "acceptForSession"}} &&
                detail::encodePermissionsRequestApprovalResponse(permissions, error) ==
                    codex::Json{{"permissions",
                                 {{"fileSystem",
                                   {{"entries",
                                     codex::Json::array(
                                         {{{"access", "write"}, {"path", {{"type", "special"}, {"value", {{"kind", "slash_tmp"}}}}}}})},
                                    {"globScanMaxDepth", std::numeric_limits<std::uint64_t>::max()}}},
                                  {"network", nullptr}}},
                                {"scope", "session"},
                                {"strictAutoReview", false}} &&
                error.empty(),
            "all five concrete server responses encode exact, distinct schemas");

        const std::vector<std::pair<typed::FileChangeApprovalDecision, std::string>> fileDecisions{
            {typed::FileChangeApprovalDecision::accept(), "accept"},
            {typed::FileChangeApprovalDecision::acceptForSession(), "acceptForSession"},
            {typed::FileChangeApprovalDecision::decline(), "decline"},
            {typed::FileChangeApprovalDecision::cancel(), "cancel"},
        };
        std::size_t fileDecisionCount = 0;
        for (const auto& [decision, wire] : fileDecisions) {
            if (detail::encodeFileChangeRequestApprovalResponse({decision}, error) == codex::Json{{"decision", wire}} && error.empty()) {
                ++fileDecisionCount;
            }
        }
        result.expectEqual(fileDecisions.size(), fileDecisionCount, "all four file-change response decisions encode exactly");

        const typed::PermissionsRequestApprovalResponse defaults{{}, std::nullopt, typed::OptionalNullable<bool>::explicitNull()};
        result.expectTrue(detail::encodePermissionsRequestApprovalResponse(defaults, error) ==
                              codex::Json{{"permissions", codex::Json::object()}, {"strictAutoReview", nullptr}},
                          "permission response preserves omitted default scope and explicit-null review flag");
    }

    void testPermissionProfiles(tests::support::TestResult& result) {
        std::string error;
        const typed::PermissionProfileListParams omitted;
        result.expectTrue(detail::encodePermissionProfileListParams(omitted, error) == codex::Json::object() && error.empty(),
                          "permissionProfile/list minimum request omits every optional field");

        const typed::PermissionProfileListParams full{
            typed::OptionalNullable<std::string>::explicitNull(),
            typed::OptionalNullable<std::string>::withValue("./synthetic/../profile-cwd"),
            typed::OptionalNullable<std::uint32_t>::withValue(std::numeric_limits<std::uint32_t>::max())};
        result.expectTrue(detail::encodePermissionProfileListParams(full, error) ==
                                  codex::Json{{"cursor", nullptr},
                                              {"cwd", "./synthetic/../profile-cwd"},
                                              {"limit", std::numeric_limits<std::uint32_t>::max()}} &&
                              error.empty(),
                          "permissionProfile/list preserves null, path bytes, and uint32 upper bound");

        const typed::PermissionProfileListParams complementaryNulls{typed::OptionalNullable<std::string>::withValue("synthetic-cursor"),
                                                                    typed::OptionalNullable<std::string>::explicitNull(),
                                                                    typed::OptionalNullable<std::uint32_t>::explicitNull()};
        result.expectTrue(detail::encodePermissionProfileListParams(complementaryNulls, error) ==
                                  codex::Json{{"cursor", "synthetic-cursor"}, {"cwd", nullptr}, {"limit", nullptr}} &&
                              error.empty(),
                          "permissionProfile/list preserves the complementary value/null states");

        const codex::Json wire{
            {"data",
             codex::Json::array({{{"allowed", true}, {"description", nullptr}, {"id", "synthetic-profile-a"}, {"futureSummary", 1}},
                                 {{"allowed", false}, {"description", "synthetic profile description"}, {"id", "synthetic-profile-b"}}})},
            {"nextCursor", "synthetic-next"},
            {"futureResponse", true},
        };
        const auto decoded = detail::decodePermissionProfileListResponse(wire, error);
        result.expectTrue(decoded && error.empty() && decoded->data.size() == 2 && decoded->data[0].allowed &&
                              decoded->data[0].description.isNull() && decoded->data[0].raw.value("futureSummary", 0) == 1 &&
                              !decoded->data[1].allowed &&
                              decoded->data[1].description.value == std::optional<std::string>{"synthetic profile description"} &&
                              decoded->nextCursor.value == std::optional<std::string>{"synthetic-next"} && decoded->raw == wire,
                          "permissionProfile/list response preserves ordering, nullability, and raw extensions");

        const auto empty = detail::decodePermissionProfileListResponse({{"data", codex::Json::array()}, {"nextCursor", nullptr}}, error);
        result.expectTrue(empty && empty->data.empty() && empty->nextCursor.isNull(),
                          "permissionProfile/list accepts an empty page and null cursor");
        const auto omittedCursor = detail::decodePermissionProfileListResponse({{"data", codex::Json::array()}}, error);
        result.expectTrue(omittedCursor && omittedCursor->nextCursor.isOmitted(),
                          "permissionProfile/list distinguishes an omitted next cursor");

        const auto missing = detail::decodePermissionProfileListResponse({{"nextCursor", nullptr}}, error);
        result.expectTrue(!missing && error.find("$.data") != std::string::npos, "permissionProfile/list rejects missing required data");
    }
} // namespace

int main() {
    tests::support::TestResult result;
    testFileChangeUnion(result);
    testFileSystemPathUnions(result);
    testParsedCommandUnion(result);
    testDecisionUnions(result);
    testRequestRoots(result);
    testResponseRoots(result);
    testPermissionProfiles(result);
    return result.processResult();
}
