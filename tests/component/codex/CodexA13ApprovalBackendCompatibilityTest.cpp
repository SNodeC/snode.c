/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/BackendCommand.h"
#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "ai/openai/codex/detail/ServerRequestDecoder.h"
#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/frontend/Protocol.h"
#include "ai/openai/codex/typed/ServerRequests.h"
#include "support/TestResult.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace frontend = ai::openai::codex::frontend;
    namespace typed = ai::openai::codex::typed;

    constexpr const char* ApplyContent = "synthetic-private-apply-content";
    constexpr const char* ApplyPath = "/synthetic/private/apply.cpp";
    constexpr const char* ApplyReason = "synthetic-private-apply-reason";
    constexpr const char* ExecArg = "synthetic-private-exec-argument";
    constexpr const char* ExecCwd = "/synthetic/private/exec-cwd";
    constexpr const char* ExecReason = "synthetic-private-exec-reason";
    constexpr const char* PermissionPath = "/synthetic/private/permission-root";
    constexpr const char* PermissionReason = "synthetic-private-permission-reason";

    struct RequestCase {
        std::uint64_t pendingId;
        std::uint64_t token;
        const char* method;
        codex::Json params;
    };

    codex::ServerRequest serverRequest(const RequestCase& testCase) {
        codex::Json raw{
            {"jsonrpc", "2.0"},
            {"id", "synthetic-request-" + std::to_string(testCase.pendingId)},
            {"method", testCase.method},
            {"params", testCase.params},
            {"futureEnvelopeOnly", "must-not-reach-frontend"},
        };
        return {
            codex::ServerRequestId{"synthetic-request-" + std::to_string(testCase.pendingId)},
            testCase.method,
            testCase.params,
            std::move(raw),
            codex::ServerRequestToken{testCase.token},
        };
    }

    const backend::PendingRequestSnapshot* findPending(const backend::Snapshot& snapshot, std::uint64_t id) {
        for (const backend::PendingRequestSnapshot& pending : snapshot.pendingRequests) {
            if (pending.id == backend::PendingRequestId{id}) {
                return &pending;
            }
        }
        return nullptr;
    }

    codex::Json pendingFrontendJson(const backend::PendingRequestSnapshot& pending) {
        codex::Json encoded{
            {"id", std::to_string(pending.id.value())},
            {"type", pending.type},
            {"details", pending.details},
        };
        if (pending.threadId) {
            encoded["threadId"] = *pending.threadId;
        }
        if (pending.turnId) {
            encoded["turnId"] = *pending.turnId;
        }
        if (pending.itemId) {
            encoded["itemId"] = *pending.itemId;
        }
        return encoded;
    }

    std::string serializePendingSnapshot(const backend::Snapshot& snapshot, tests::support::TestResult& result) {
        codex::Json pending = codex::Json::array();
        for (const backend::PendingRequestSnapshot& request : snapshot.pendingRequests) {
            pending.push_back(pendingFrontendJson(request));
        }
        const frontend::Snapshot message{
            frontend::SequenceNumber{snapshot.sequence.value()},
            codex::Json{{"pendingRequests", std::move(pending)}},
            codex::Json::object(),
        };
        const auto serialized = frontend::Codec::serializeServer(frontend::ServerMessage{message});
        result.expectTrue(serialized.hasValue(), "Frontend Protocol v1 serializes the approval compatibility snapshot");
        if (!serialized) {
            return {};
        }

        const codex::Json expected{
            {"kind", "snapshot"},
            {"protocol", frontend::ProtocolIdentity},
            {"sequence", snapshot.sequence.value()},
            {"state",
             {{"pendingRequests",
               [&snapshot]() {
                   codex::Json values = codex::Json::array();
                   for (const backend::PendingRequestSnapshot& request : snapshot.pendingRequests) {
                       values.push_back(pendingFrontendJson(request));
                   }
                   return values;
               }()}}},
            {"version", frontend::ProtocolVersion},
        };
        result.expectTrue(expected.dump() == serialized.value(),
                          "approval compatibility snapshot retains the exact existing Frontend Protocol v1 envelope and request shape");
        return serialized.value();
    }

    bool hasExpectedTypedAlternative(std::string_view method, const typed::TypedServerRequest& request) {
        if (method == "applyPatchApproval") {
            return std::holds_alternative<typed::ApplyPatchApprovalRequest>(request);
        }
        if (method == "execCommandApproval") {
            return std::holds_alternative<typed::ExecCommandApprovalRequest>(request);
        }
        return std::holds_alternative<typed::PermissionsApprovalRequest>(request);
    }

    void testNewRequestsRetainGenericRedactedFrontendBoundary(tests::support::TestResult& result) {
        const std::array<RequestCase, 3> cases{{
            {
                1,
                101,
                "applyPatchApproval",
                {
                    {"callId", "synthetic-private-apply-call"},
                    {"conversationId", "synthetic-private-apply-conversation"},
                    {"fileChanges", {{ApplyPath, {{"type", "add"}, {"content", ApplyContent}}}}},
                    {"grantRoot", "/synthetic/private/apply-root"},
                    {"reason", ApplyReason},
                    {"futureSafeField", "apply-safe"},
                },
            },
            {
                2,
                102,
                "execCommandApproval",
                {
                    {"approvalId", "synthetic-private-approval-id"},
                    {"callId", "synthetic-private-exec-call"},
                    {"command", codex::Json::array({"synthetic-tool", ExecArg})},
                    {"conversationId", "synthetic-private-exec-conversation"},
                    {"cwd", ExecCwd},
                    {"parsedCmd", codex::Json::array({{{"type", "unknown"}, {"cmd", "synthetic-private-parsed-command"}}})},
                    {"reason", ExecReason},
                    {"futureSafeField", "exec-safe"},
                },
            },
            {
                3,
                103,
                "item/permissions/requestApproval",
                {
                    {"cwd", PermissionPath},
                    {"environmentId", "synthetic-private-environment"},
                    {"itemId", "synthetic-private-permission-item"},
                    {"permissions",
                     {{"fileSystem",
                       {{"entries", codex::Json::array({{{"access", "write"}, {"path", {{"type", "path"}, {"path", PermissionPath}}}}})},
                        {"read", codex::Json::array({PermissionPath})},
                        {"write", codex::Json::array({PermissionPath})}}},
                      {"network", {{"enabled", true}}}}},
                    {"reason", PermissionReason},
                    {"startedAtMs", 1700000000000LL},
                    {"threadId", "synthetic-private-permission-thread"},
                    {"turnId", "synthetic-private-permission-turn"},
                    {"futureSafeField", "permission-safe"},
                },
            },
        }};

        backend::Reducer reducer;
        backend::BackendState state;
        const backend::Snapshot before = backend::makeSnapshot(state);

        for (const RequestCase& testCase : cases) {
            typed::TypedServerRequest decoded = detail::decodeServerRequest(serverRequest(testCase));
            result.expectTrue(hasExpectedTypedAlternative(testCase.method, decoded),
                              std::string(testCase.method) + " decodes through its appended typed request alternative");
            const backend::Reduction reduction =
                reducer.apply(state, backend::PendingRequestAdded{{backend::PendingRequestId{testCase.pendingId}, std::move(decoded), 7}});
            result.expectTrue(reduction.changed && reduction.flushImmediately,
                              std::string(testCase.method) + " reuses the existing interactive pending-request lifecycle");
        }

        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        const backend::PendingRequestSnapshot* apply = findPending(snapshot, 1);
        const backend::PendingRequestSnapshot* exec = findPending(snapshot, 2);
        const backend::PendingRequestSnapshot* permissions = findPending(snapshot, 3);
        result.expectTrue(apply && exec && permissions && apply->type == "unknown" && exec->type == "unknown" &&
                              permissions->type == "unknown",
                          "new approval request alternatives retain the existing generic frontend request type");

        if (apply && exec && permissions) {
            const codex::Json& applyParams = apply->details["params"];
            const codex::Json& execParams = exec->details["params"];
            const codex::Json& permissionParams = permissions->details["params"];
            result.expectTrue(
                apply->details.value("method", "") == "applyPatchApproval" && apply->details.value("sensitiveFieldsRedacted", false) &&
                    applyParams.value("callId", "") == "[redacted]" && applyParams.value("conversationId", "") == "[redacted]" &&
                    applyParams.value("fileChanges", "") == "[redacted]" && applyParams.value("grantRoot", "") == "[redacted]" &&
                    applyParams.value("reason", "") == "[redacted]" && applyParams.value("futureSafeField", "") == "apply-safe",
                "legacy patch approval keeps its generic shape while redacting patch, path, reason, and correlation data");
            result.expectTrue(
                exec->details.value("method", "") == "execCommandApproval" && exec->details.value("sensitiveFieldsRedacted", false) &&
                    execParams.value("approvalId", "") == "[redacted]" && execParams.value("callId", "") == "[redacted]" &&
                    execParams.value("command", "") == "[redacted]" && execParams.value("conversationId", "") == "[redacted]" &&
                    execParams.value("cwd", "") == "[redacted]" && execParams.value("parsedCmd", "") == "[redacted]" &&
                    execParams.value("reason", "") == "[redacted]" && execParams.value("futureSafeField", "") == "exec-safe",
                "legacy command approval keeps its generic shape while redacting argv, paths, reason, and correlation data");
            result.expectTrue(
                permissions->details.value("method", "") == "item/permissions/requestApproval" &&
                    permissions->details.value("sensitiveFieldsRedacted", false) && permissionParams.value("cwd", "") == "[redacted]" &&
                    permissionParams.value("environmentId", "") == "[redacted]" && permissionParams.value("itemId", "") == "[redacted]" &&
                    permissionParams.value("permissions", "") == "[redacted]" && permissionParams.value("reason", "") == "[redacted]" &&
                    permissionParams.value("threadId", "") == "[redacted]" && permissionParams.value("turnId", "") == "[redacted]" &&
                    permissionParams.value("startedAtMs", 0LL) == 1700000000000LL &&
                    permissionParams.value("futureSafeField", "") == "permission-safe",
                "permission approval keeps its generic shape while redacting paths, requested permissions, reason, and identities");
        }

        const std::string frontendBytes = serializePendingSnapshot(snapshot, result);
        result.expectTrue(
            frontendBytes.find(ApplyContent) == std::string::npos && frontendBytes.find(ApplyPath) == std::string::npos &&
                frontendBytes.find(ApplyReason) == std::string::npos && frontendBytes.find(ExecArg) == std::string::npos &&
                frontendBytes.find(ExecCwd) == std::string::npos && frontendBytes.find(ExecReason) == std::string::npos &&
                frontendBytes.find(PermissionPath) == std::string::npos && frontendBytes.find(PermissionReason) == std::string::npos &&
                frontendBytes.find("must-not-reach-frontend") == std::string::npos &&
                frontendBytes.find("apply-safe") != std::string::npos && frontendBytes.find("exec-safe") != std::string::npos &&
                frontendBytes.find("permission-safe") != std::string::npos,
            "serialized frontend snapshot bytes contain no synthetic A1.3 approval-sensitive values or raw envelopes");

        backend::BackendState withoutPending = state;
        withoutPending.pendingRequests.clear();
        backend::Snapshot after = backend::makeSnapshot(withoutPending);
        after.sequence = before.sequence;
        result.expectTrue(
            after == before,
            "new typed approvals add no semantic command, patch, permission, or approval state beyond the existing pending map");
    }

    void testEstablishedApprovalProjectionIsUnchanged(tests::support::TestResult& result) {
        const RequestCase command{
            10,
            110,
            "item/commandExecution/requestApproval",
            {
                {"command", "synthetic-established-command"},
                {"cwd", "/synthetic/established-command-cwd"},
                {"itemId", "established-command-item"},
                {"reason", "synthetic-established-command-reason"},
                {"startedAtMs", 11},
                {"threadId", "established-command-thread"},
                {"turnId", "established-command-turn"},
            },
        };
        const RequestCase file{
            11,
            111,
            "item/fileChange/requestApproval",
            {
                {"grantRoot", "/synthetic/established-file-root"},
                {"itemId", "established-file-item"},
                {"reason", "synthetic-established-file-reason"},
                {"startedAtMs", 12},
                {"threadId", "established-file-thread"},
                {"turnId", "established-file-turn"},
            },
        };

        typed::TypedServerRequest commandRequest = detail::decodeServerRequest(serverRequest(command));
        typed::TypedServerRequest fileRequest = detail::decodeServerRequest(serverRequest(file));
        result.expectTrue(std::holds_alternative<typed::CommandApprovalRequest>(commandRequest) &&
                              std::holds_alternative<typed::FileChangeApprovalRequest>(fileRequest),
                          "the two established approval identities retain their source-level typed alternatives");

        backend::Reducer reducer;
        backend::BackendState state;
        reducer.apply(state, backend::PendingRequestAdded{{backend::PendingRequestId{10}, std::move(commandRequest), 7}});
        reducer.apply(state, backend::PendingRequestAdded{{backend::PendingRequestId{11}, std::move(fileRequest), 7}});
        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        const backend::PendingRequestSnapshot* commandSnapshot = findPending(snapshot, 10);
        const backend::PendingRequestSnapshot* fileSnapshot = findPending(snapshot, 11);

        result.expectTrue(
            commandSnapshot && commandSnapshot->type == "command_approval" && commandSnapshot->threadId == "established-command-thread" &&
                commandSnapshot->turnId == "established-command-turn" && commandSnapshot->itemId == "established-command-item" &&
                commandSnapshot->details.value("command", "") == "synthetic-established-command" &&
                commandSnapshot->details.value("cwd", "") == "/synthetic/established-command-cwd" &&
                commandSnapshot->details.value("reason", "") == "synthetic-established-command-reason" &&
                !commandSnapshot->details.contains("method") && !commandSnapshot->details.contains("params"),
            "schema completion preserves the established command-approval frontend projection");
        result.expectTrue(fileSnapshot && fileSnapshot->type == "file_change_approval" &&
                              fileSnapshot->threadId == "established-file-thread" && fileSnapshot->turnId == "established-file-turn" &&
                              fileSnapshot->itemId == "established-file-item" &&
                              fileSnapshot->details.value("grantRoot", "") == "/synthetic/established-file-root" &&
                              fileSnapshot->details.value("reason", "") == "synthetic-established-file-reason" &&
                              !fileSnapshot->details.contains("method") && !fileSnapshot->details.contains("params"),
                          "schema completion preserves the established file-change-approval frontend projection");
    }
} // namespace

int main() {
    static_assert(std::variant_size_v<backend::BackendCommand> == 15);
    static_assert(!std::is_constructible_v<backend::BackendCommand, typed::ApplyPatchApprovalRequest>);
    static_assert(!std::is_constructible_v<backend::BackendCommand, typed::ExecCommandApprovalRequest>);
    static_assert(!std::is_constructible_v<backend::BackendCommand, typed::PermissionsApprovalRequest>);
    static_assert(std::is_same_v<decltype(backend::BackendState::pendingRequests),
                                 std::map<backend::PendingRequestId, backend::PendingRequestState>>);
    static_assert(std::is_same_v<std::variant_alternative_t<0, typed::TypedServerRequest>, typed::CommandApprovalRequest>);
    static_assert(std::is_same_v<std::variant_alternative_t<1, typed::TypedServerRequest>, typed::FileChangeApprovalRequest>);
    static_assert(std::is_same_v<std::variant_alternative_t<4, typed::TypedServerRequest>, typed::UnknownServerRequest>);
    static_assert(std::is_same_v<std::variant_alternative_t<5, typed::TypedServerRequest>, typed::ApplyPatchApprovalRequest>);

    tests::support::TestResult result;
    testNewRequestsRetainGenericRedactedFrontendBoundary(result);
    testEstablishedApprovalProjectionIsUnchanged(result);
    return result.processResult();
}
