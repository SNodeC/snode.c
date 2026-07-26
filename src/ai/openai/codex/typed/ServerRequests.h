/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_SERVERREQUESTS_H
#define AI_OPENAI_CODEX_TYPED_SERVERREQUESTS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Accounts.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Types.h"

#include <compare>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct NetworkApprovalProtocol {
        std::string value;

        static NetworkApprovalProtocol http() {
            return {"http"};
        }

        static NetworkApprovalProtocol https() {
            return {"https"};
        }

        static NetworkApprovalProtocol socks5Tcp() {
            return {"socks5Tcp"};
        }

        static NetworkApprovalProtocol socks5Udp() {
            return {"socks5Udp"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "http" || value == "https" || value == "socks5Tcp" || value == "socks5Udp";
        }

        auto operator<=>(const NetworkApprovalProtocol&) const = default;
    };

    struct NetworkPolicyRuleAction {
        std::string value;

        static NetworkPolicyRuleAction allow() {
            return {"allow"};
        }

        static NetworkPolicyRuleAction deny() {
            return {"deny"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "allow" || value == "deny";
        }

        auto operator<=>(const NetworkPolicyRuleAction&) const = default;
    };

    struct NetworkPolicyAmendment {
        NetworkPolicyRuleAction action;
        std::string host;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct NetworkApprovalContext {
        std::string host;
        NetworkApprovalProtocol protocol;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct RootFileSystemSpecialPath {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct MinimalFileSystemSpecialPath {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ProjectRootsFileSystemSpecialPath {
        OptionalNullable<std::string> subpath;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TmpdirFileSystemSpecialPath {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct SlashTmpFileSystemSpecialPath {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    // "unknown" is a pinned, known wire alternative. It is distinct from the
    // genuinely future alternative below.
    struct UnknownFileSystemSpecialPath {
        std::string path;
        OptionalNullable<std::string> subpath;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct UnrecognizedFileSystemSpecialPath {
        std::optional<std::string> kind;
        Json raw = Json::object();
        DecodeDiagnostic diagnostic;
    };

    using FileSystemSpecialPath = std::variant<RootFileSystemSpecialPath,
                                               MinimalFileSystemSpecialPath,
                                               ProjectRootsFileSystemSpecialPath,
                                               TmpdirFileSystemSpecialPath,
                                               SlashTmpFileSystemSpecialPath,
                                               UnknownFileSystemSpecialPath,
                                               UnrecognizedFileSystemSpecialPath>;

    struct PathFileSystemPath {
        std::string path;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct GlobPatternFileSystemPath {
        std::string pattern;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct SpecialFileSystemPath {
        FileSystemSpecialPath value;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct UnrecognizedFileSystemPath {
        std::optional<std::string> type;
        Json raw = Json::object();
        DecodeDiagnostic diagnostic;
    };

    using FileSystemPath = std::variant<PathFileSystemPath, GlobPatternFileSystemPath, SpecialFileSystemPath, UnrecognizedFileSystemPath>;

    struct FileSystemAccessMode {
        std::string value;

        static FileSystemAccessMode read() {
            return {"read"};
        }

        static FileSystemAccessMode write() {
            return {"write"};
        }

        static FileSystemAccessMode deny() {
            return {"deny"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "read" || value == "write" || value == "deny";
        }

        auto operator<=>(const FileSystemAccessMode&) const = default;
    };

    struct FileSystemSandboxEntry {
        FileSystemAccessMode access;
        FileSystemPath path;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct AdditionalFileSystemPermissions {
        OptionalNullable<std::vector<FileSystemSandboxEntry>> entries;
        OptionalNullable<std::uint64_t> globScanMaxDepth;
        OptionalNullable<std::vector<std::string>> read;
        OptionalNullable<std::vector<std::string>> write;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct AdditionalNetworkPermissions {
        OptionalNullable<bool> enabled;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct RequestPermissionProfile {
        OptionalNullable<AdditionalFileSystemPermissions> fileSystem;
        OptionalNullable<AdditionalNetworkPermissions> network;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct GrantedPermissionProfile {
        OptionalNullable<AdditionalFileSystemPermissions> fileSystem;
        OptionalNullable<AdditionalNetworkPermissions> network;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct PermissionGrantScope {
        std::string value;

        static PermissionGrantScope turn() {
            return {"turn"};
        }

        static PermissionGrantScope session() {
            return {"session"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "turn" || value == "session";
        }

        auto operator<=>(const PermissionGrantScope&) const = default;
    };

    struct AddFileChange {
        std::string content;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct DeleteFileChange {
        std::string content;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct UpdateFileChange {
        OptionalNullable<std::string> movePath;
        std::string unifiedDiff;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct UnrecognizedFileChange {
        std::optional<std::string> type;
        Json raw = Json::object();
        DecodeDiagnostic diagnostic;
    };

    using FileChange = std::variant<AddFileChange, DeleteFileChange, UpdateFileChange, UnrecognizedFileChange>;

    struct ReadParsedCommand {
        std::string command;
        std::string name;
        std::string path;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ListFilesParsedCommand {
        std::string command;
        OptionalNullable<std::string> path;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct SearchParsedCommand {
        std::string command;
        OptionalNullable<std::string> path;
        OptionalNullable<std::string> query;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    // "unknown" is a pinned, known ParsedCommand alternative.
    struct UnknownParsedCommand {
        std::string command;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct UnrecognizedParsedCommand {
        std::optional<std::string> type;
        Json raw = Json::object();
        DecodeDiagnostic diagnostic;
    };

    using ParsedCommand =
        std::variant<ReadParsedCommand, ListFilesParsedCommand, SearchParsedCommand, UnknownParsedCommand, UnrecognizedParsedCommand>;

    struct AcceptCommandExecutionApprovalDecision {};

    struct AcceptForSessionCommandExecutionApprovalDecision {};

    struct AcceptWithExecpolicyAmendmentCommandExecutionApprovalDecision {
        std::vector<std::string> execpolicyAmendment;
    };

    struct ApplyNetworkPolicyAmendmentCommandExecutionApprovalDecision {
        NetworkPolicyAmendment networkPolicyAmendment;
    };

    struct DeclineCommandExecutionApprovalDecision {};

    struct CancelCommandExecutionApprovalDecision {};

    struct UnrecognizedCommandExecutionApprovalDecision {
        std::optional<std::string> variant;
        Json raw = nullptr;
        DecodeDiagnostic diagnostic;
    };

    using CommandExecutionApprovalDecision = std::variant<AcceptCommandExecutionApprovalDecision,
                                                          AcceptForSessionCommandExecutionApprovalDecision,
                                                          AcceptWithExecpolicyAmendmentCommandExecutionApprovalDecision,
                                                          ApplyNetworkPolicyAmendmentCommandExecutionApprovalDecision,
                                                          DeclineCommandExecutionApprovalDecision,
                                                          CancelCommandExecutionApprovalDecision,
                                                          UnrecognizedCommandExecutionApprovalDecision>;

    struct ApprovedReviewDecision {};

    struct ApprovedExecpolicyAmendmentReviewDecision {
        std::vector<std::string> proposedExecpolicyAmendment;
    };

    struct ApprovedForSessionReviewDecision {};

    struct NetworkPolicyAmendmentReviewDecision {
        NetworkPolicyAmendment networkPolicyAmendment;
    };

    struct DeniedReviewDecision {};

    struct TimedOutReviewDecision {};

    struct AbortReviewDecision {};

    struct UnrecognizedReviewDecision {
        std::optional<std::string> variant;
        Json raw = nullptr;
        DecodeDiagnostic diagnostic;
    };

    using ReviewDecision = std::variant<ApprovedReviewDecision,
                                        ApprovedExecpolicyAmendmentReviewDecision,
                                        ApprovedForSessionReviewDecision,
                                        NetworkPolicyAmendmentReviewDecision,
                                        DeniedReviewDecision,
                                        TimedOutReviewDecision,
                                        AbortReviewDecision,
                                        UnrecognizedReviewDecision>;

    struct FileChangeApprovalDecision {
        std::string value;

        static FileChangeApprovalDecision accept() {
            return {"accept"};
        }

        static FileChangeApprovalDecision acceptForSession() {
            return {"acceptForSession"};
        }

        static FileChangeApprovalDecision decline() {
            return {"decline"};
        }

        static FileChangeApprovalDecision cancel() {
            return {"cancel"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "accept" || value == "acceptForSession" || value == "decline" || value == "cancel";
        }

        auto operator<=>(const FileChangeApprovalDecision&) const = default;
    };

    struct ApplyPatchApprovalParams {
        ResponseCallId callId;
        ThreadId conversationId;
        std::map<std::string, FileChange> fileChanges;
        OptionalNullable<std::string> grantRoot;
        OptionalNullable<std::string> reason;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ApplyPatchApprovalResponse {
        ReviewDecision decision;
    };

    struct ExecCommandApprovalParams {
        OptionalNullable<std::string> approvalId;
        ResponseCallId callId;
        std::vector<std::string> command;
        ThreadId conversationId;
        std::string cwd;
        std::vector<ParsedCommand> parsedCommand;
        OptionalNullable<std::string> reason;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ExecCommandApprovalResponse {
        ReviewDecision decision;
    };

    struct CommandExecutionRequestApprovalParams {
        OptionalNullable<std::string> approvalId;
        OptionalNullable<std::string> command;
        OptionalNullable<std::vector<CommandAction>> commandActions;
        OptionalNullable<LegacyAppPathString> cwd;
        OptionalNullable<std::string> environmentId;
        ItemId itemId;
        OptionalNullable<NetworkApprovalContext> networkApprovalContext;
        OptionalNullable<std::vector<std::string>> proposedExecpolicyAmendment;
        OptionalNullable<std::vector<NetworkPolicyAmendment>> proposedNetworkPolicyAmendments;
        OptionalNullable<std::string> reason;
        std::int64_t startedAtMs = 0;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct CommandExecutionRequestApprovalResponse {
        CommandExecutionApprovalDecision decision;
    };

    struct FileChangeRequestApprovalParams {
        OptionalNullable<std::string> grantRoot;
        ItemId itemId;
        OptionalNullable<std::string> reason;
        std::int64_t startedAtMs = 0;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct FileChangeRequestApprovalResponse {
        FileChangeApprovalDecision decision;
    };

    struct PermissionsRequestApprovalParams {
        AbsolutePathBuf cwd;
        OptionalNullable<std::string> environmentId;
        ItemId itemId;
        RequestPermissionProfile permissions;
        OptionalNullable<std::string> reason;
        std::int64_t startedAtMs = 0;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct PermissionsRequestApprovalResponse {
        GrantedPermissionProfile permissions;
        // Omission uses the protocol's "turn" default; a present value is encoded
        // exactly and remains open to future values.
        std::optional<PermissionGrantScope> scope;
        OptionalNullable<bool> strictAutoReview;
    };

    struct CommandApprovalRequest {
        ServerRequestId requestId;
        ServerRequestToken requestToken;
        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        std::int64_t startedAtMs = 0;
        std::optional<std::string> command;
        std::optional<std::string> cwd;
        std::optional<std::string> reason;
        Json details;
        Json raw;
        // Schema-complete canonical view. The compatibility projection above
        // preserves the established source-level access pattern.
        CommandExecutionRequestApprovalParams canonicalParams;
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct FileChangeApprovalRequest {
        ServerRequestId requestId;
        ServerRequestToken requestToken;
        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        std::int64_t startedAtMs = 0;
        std::optional<std::string> reason;
        std::optional<std::string> grantRoot;
        Json raw;
        FileChangeRequestApprovalParams canonicalParams;
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct UserInputOption {
        std::string label;
        std::string description;
        Json raw;
    };

    struct UserInputQuestion {
        std::string id;
        std::string header;
        std::string prompt;
        std::vector<UserInputOption> options;
        bool allowsFreeText = false;
        bool secret = false;
        Json raw;
    };

    struct UserInputRequest {
        ServerRequestId requestId;
        ServerRequestToken requestToken;
        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        std::vector<UserInputQuestion> questions;
        std::optional<std::uint64_t> autoResolutionMs;
        Json raw;
    };

    struct AuthenticationRequest {
        ServerRequestId requestId;
        ServerRequestToken requestToken;
        std::string reason;
        std::optional<std::string> previousAccountId;
        Json raw;
        // Schema-complete canonical view. The legacy fields above remain as a
        // source-compatible projection; only canonicalParams distinguishes an
        // omitted previous account ID from an explicit null.
        ChatgptAuthTokensRefreshParams canonicalParams;
        std::vector<DecodeDiagnostic> diagnostics;
    };

    using ChatgptAuthTokensRefreshRequest = AuthenticationRequest;

    struct UnknownServerRequest {
        ServerRequestId requestId;
        ServerRequestToken requestToken;
        std::string method;
        Json params;
        Json raw;
        std::optional<std::string> decodingError;
        std::optional<DecodeDiagnostic> diagnostic;
    };

    struct ApplyPatchApprovalRequest {
        ServerRequestId requestId;
        ServerRequestToken requestToken;
        ApplyPatchApprovalParams params;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ExecCommandApprovalRequest {
        ServerRequestId requestId;
        ServerRequestToken requestToken;
        ExecCommandApprovalParams params;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct PermissionsApprovalRequest {
        ServerRequestId requestId;
        ServerRequestToken requestToken;
        PermissionsRequestApprovalParams params;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    using PermissionsRequestApprovalRequest = PermissionsApprovalRequest;

    // Existing alternatives retain their indices. New A1.3 alternatives are
    // appended after the former final UnknownServerRequest alternative.
    using TypedServerRequest = std::variant<CommandApprovalRequest,
                                            FileChangeApprovalRequest,
                                            UserInputRequest,
                                            AuthenticationRequest,
                                            UnknownServerRequest,
                                            ApplyPatchApprovalRequest,
                                            ExecCommandApprovalRequest,
                                            PermissionsApprovalRequest>;

    struct ApprovalDecision {
        std::string value;

        static ApprovalDecision accept();
        static ApprovalDecision acceptForSession();
        static ApprovalDecision decline();
        static ApprovalDecision cancel();

        auto operator<=>(const ApprovalDecision&) const = default;
    };

    struct UserInputAnswer {
        std::string questionId;
        std::vector<std::string> answers;
    };

    struct AuthenticationResponse {
        std::string accessToken;
        std::string chatgptAccountId;
        std::optional<std::string> chatgptPlanType;
    };

    class Requests {
    public:
        using SendResult = AppServerClient::RawProtocol::SendResult;
        using RequestHandler = std::function<void(const TypedServerRequest&)>;

        void setOnRequest(RequestHandler handler);

        SendResult respond(const CommandApprovalRequest& request, ApprovalDecision decision);
        SendResult respond(const CommandApprovalRequest& request, CommandExecutionRequestApprovalResponse response);
        SendResult respond(const FileChangeApprovalRequest& request, ApprovalDecision decision);
        SendResult respond(const FileChangeApprovalRequest& request, FileChangeRequestApprovalResponse response);
        SendResult respond(const ApplyPatchApprovalRequest& request, ApplyPatchApprovalResponse response);
        SendResult respond(const ExecCommandApprovalRequest& request, ExecCommandApprovalResponse response);
        SendResult respond(const PermissionsApprovalRequest& request, PermissionsRequestApprovalResponse response);
        SendResult respond(const UserInputRequest& request, std::vector<UserInputAnswer> answers);
        SendResult respondRefresh(const ChatgptAuthTokensRefreshRequest& request, ChatgptAuthTokensRefreshResponse response);
        SendResult respond(const AuthenticationRequest& request, AuthenticationResponse response);
        SendResult respondRaw(const UnknownServerRequest& request, Json result);
        SendResult reject(const UnknownServerRequest& request, ProtocolError error);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Requests(AppServerClient::RawProtocol& protocol) noexcept;

        static SendResult validationFailure(std::string message);

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_SERVERREQUESTS_H
