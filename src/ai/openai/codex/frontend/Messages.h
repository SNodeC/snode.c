/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_FRONTEND_MESSAGES_H
#define AI_OPENAI_CODEX_FRONTEND_MESSAGES_H

#include <compare>
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ai::openai::codex::frontend {

    using Json = nlohmann::json;

    class SequenceNumber {
    public:
        using Value = std::uint64_t;

        constexpr SequenceNumber() noexcept = default;

        explicit constexpr SequenceNumber(Value value) noexcept
            : sequence(value) {
        }

        [[nodiscard]] constexpr Value value() const noexcept {
            return sequence;
        }

        [[nodiscard]] static constexpr SequenceNumber maximum() noexcept {
            return SequenceNumber(std::numeric_limits<Value>::max());
        }

        auto operator<=>(const SequenceNumber&) const = default;

    private:
        Value sequence = 0;
    };

    enum class SessionRole { Observer, Controller };
    enum class SyncMode { Replay, Snapshot };

    // These string values are part of Frontend Protocol v1 and must remain
    // stable. Command failures and frontend-local protocol failures share one
    // enum so applications can implement one exhaustive error policy.
    enum class ErrorCode {
        PermissionDenied,
        InvalidCommand,
        NotFound,
        Conflict,
        LocalSubmissionFailure,
        TypedDecodingFailure,
        RemoteAppServerError,
        Cancelled,
        BackendUnavailable,
        DuplicateRequestId,
        MalformedJson,
        WrongProtocol,
        UnsupportedVersion,
        MissingField,
        InvalidField,
        UnknownKind,
        UnknownMethod,
        FrameTooLarge,
        CapacityExceeded,
        SequenceOverflow,
        ReplayGap,
        InternalError
    };

    [[nodiscard]] std::string_view toString(SessionRole role) noexcept;
    [[nodiscard]] std::string_view toString(SyncMode mode) noexcept;
    [[nodiscard]] std::string_view toString(ErrorCode code) noexcept;
    [[nodiscard]] std::optional<SessionRole> sessionRoleFromString(std::string_view value) noexcept;
    [[nodiscard]] std::optional<SyncMode> syncModeFromString(std::string_view value) noexcept;
    [[nodiscard]] std::optional<ErrorCode> errorCodeFromString(std::string_view value) noexcept;

    struct Hello {
        std::optional<SequenceNumber> resumeAfter;
        Json extensions = Json::object();

        bool operator==(const Hello&) const = default;
    };

    struct ControllerAcquire {
        bool operator==(const ControllerAcquire&) const = default;
    };
    struct ControllerRelease {
        bool operator==(const ControllerRelease&) const = default;
    };
    struct SnapshotGet {
        bool operator==(const SnapshotGet&) const = default;
    };

    struct ReplayAfter {
        SequenceNumber after;

        bool operator==(const ReplayAfter&) const = default;
    };

    struct ThreadStart {
        std::optional<std::string> cwd;
        std::optional<std::string> model;
        std::optional<std::string> modelProvider;
        std::optional<std::string> approvalPolicy;
        std::optional<std::string> sandboxMode;
        std::optional<bool> ephemeral;

        bool operator==(const ThreadStart&) const = default;
    };

    struct ThreadResume {
        std::string threadId;
        std::optional<std::string> cwd;
        std::optional<std::string> model;
        std::optional<std::string> modelProvider;
        std::optional<std::string> approvalPolicy;
        std::optional<std::string> sandboxMode;

        bool operator==(const ThreadResume&) const = default;
    };

    struct ThreadList {
        std::optional<std::string> cursor;
        std::optional<std::uint32_t> limit;
        std::optional<bool> archived;
        std::optional<std::string> searchTerm;

        bool operator==(const ThreadList&) const = default;
    };

    struct ThreadRead {
        std::string threadId;
        std::optional<bool> includeTurns;

        bool operator==(const ThreadRead&) const = default;
    };

    struct TextInput {
        std::string text;
        Json extensions = Json::object();

        bool operator==(const TextInput&) const = default;
    };

    struct ImageUrlInput {
        std::string url;
        std::optional<std::string> detail;
        Json extensions = Json::object();

        bool operator==(const ImageUrlInput&) const = default;
    };

    struct LocalImageInput {
        std::string path;
        std::optional<std::string> detail;
        Json extensions = Json::object();

        bool operator==(const LocalImageInput&) const = default;
    };

    struct SkillInput {
        std::string name;
        std::string path;
        Json extensions = Json::object();

        bool operator==(const SkillInput&) const = default;
    };

    struct MentionInput {
        std::string name;
        std::string path;
        Json extensions = Json::object();

        bool operator==(const MentionInput&) const = default;
    };

    using TurnInput = std::variant<TextInput, ImageUrlInput, LocalImageInput, SkillInput, MentionInput>;

    using SandboxNetworkAccess = std::variant<bool, std::string>;

    struct SandboxPolicy {
        std::string type;
        std::optional<SandboxNetworkAccess> networkAccess;
        std::vector<std::string> writableRoots;
        std::optional<bool> excludeTmpdirEnvVar;
        std::optional<bool> excludeSlashTmp;
        Json extensions = Json::object();

        bool operator==(const SandboxPolicy&) const = default;
    };

    struct TurnStart {
        std::string threadId;
        std::vector<TurnInput> input;
        std::optional<std::string> cwd;
        std::optional<std::string> model;
        std::optional<std::string> reasoningEffort;
        std::optional<std::string> approvalPolicy;
        std::optional<SandboxPolicy> sandboxPolicy;

        bool operator==(const TurnStart&) const = default;
    };

    struct TurnInterrupt {
        std::string threadId;
        std::string turnId;

        bool operator==(const TurnInterrupt&) const = default;
    };

    struct ApprovalRespond {
        std::string pendingRequestId;
        std::string decision;

        bool operator==(const ApprovalRespond&) const = default;
    };

    struct UserInputAnswer {
        std::string questionId;
        std::vector<std::string> answers;

        bool operator==(const UserInputAnswer&) const = default;
    };

    struct UserInputRespond {
        std::string pendingRequestId;
        std::vector<UserInputAnswer> answers;

        bool operator==(const UserInputRespond&) const = default;
    };

    struct AuthenticationRespond {
        std::string pendingRequestId;
        std::string accessToken;
        std::string chatgptAccountId;
        std::optional<std::string> chatgptPlanType;

        bool operator==(const AuthenticationRespond&) const = default;
    };

    struct UnknownRequestRespond {
        std::string pendingRequestId;
        Json result = nullptr;

        bool operator==(const UnknownRequestRespond&) const = default;
    };

    struct UnknownRequestReject {
        std::string pendingRequestId;
        std::int64_t code = 0;
        std::string message;
        std::optional<Json> data;

        bool operator==(const UnknownRequestReject&) const = default;
    };

    enum class CommandMethod {
        ControllerAcquire,
        ControllerRelease,
        SnapshotGet,
        EventsReplay,
        ThreadStart,
        ThreadResume,
        ThreadList,
        ThreadRead,
        TurnStart,
        TurnInterrupt,
        ApprovalRespond,
        UserInputRespond,
        AuthenticationRespond,
        UnknownRequestRespond,
        UnknownRequestReject
    };

    using CommandParameters = std::variant<ControllerAcquire,
                                           ControllerRelease,
                                           SnapshotGet,
                                           ReplayAfter,
                                           ThreadStart,
                                           ThreadResume,
                                           ThreadList,
                                           ThreadRead,
                                           TurnStart,
                                           TurnInterrupt,
                                           ApprovalRespond,
                                           UserInputRespond,
                                           AuthenticationRespond,
                                           UnknownRequestRespond,
                                           UnknownRequestReject>;

    [[nodiscard]] std::string_view toString(CommandMethod method) noexcept;
    [[nodiscard]] std::optional<CommandMethod> commandMethodFromString(std::string_view value) noexcept;
    [[nodiscard]] CommandMethod commandMethod(const CommandParameters& parameters) noexcept;

    struct Command {
        std::string requestId;
        CommandParameters parameters;
        Json extensions = Json::object();
        Json parameterExtensions = Json::object();

        bool operator==(const Command&) const = default;
    };

    using ClientMessage = std::variant<Hello, Command>;

    struct Welcome {
        std::string sessionId;
        SessionRole role = SessionRole::Observer;
        SequenceNumber currentSequence;
        SyncMode syncMode = SyncMode::Snapshot;
        Json extensions = Json::object();

        bool operator==(const Welcome&) const = default;
    };

    struct SyncComplete {
        SequenceNumber sequence;
        Json extensions = Json::object();

        bool operator==(const SyncComplete&) const = default;
    };

    struct Snapshot {
        SequenceNumber sequence;
        Json state = Json::object();
        Json extensions = Json::object();

        bool operator==(const Snapshot&) const = default;
    };

    struct FrontendEvent {
        SequenceNumber sequence;
        std::string type;
        Json data = Json::object();
        Json extensions = Json::object();

        bool operator==(const FrontendEvent&) const = default;
    };

    struct EventBatch {
        SequenceNumber fromSequence;
        SequenceNumber toSequence;
        std::vector<FrontendEvent> events;
        Json extensions = Json::object();

        bool operator==(const EventBatch&) const = default;
    };

    struct CommandError {
        ErrorCode code = ErrorCode::InternalError;
        std::string message;
        std::optional<Json> details;
        Json extensions = Json::object();

        bool operator==(const CommandError&) const = default;
    };

    struct Response {
        std::string requestId;
        bool ok = false;
        std::optional<Json> result;
        std::optional<CommandError> error;
        Json extensions = Json::object();

        [[nodiscard]] static Response success(std::string requestId, Json result = Json::object());
        [[nodiscard]] static Response failure(std::string requestId, CommandError error);

        bool operator==(const Response&) const = default;
    };

    struct ProtocolErrorMessage {
        ErrorCode code = ErrorCode::InvalidCommand;
        std::string message;
        std::vector<std::uint32_t> supportedVersions;
        bool closeConnection = false;
        std::optional<std::string> requestId;
        std::optional<Json> details;
        Json extensions = Json::object();

        bool operator==(const ProtocolErrorMessage&) const = default;
    };

    using ServerMessage = std::variant<Welcome, SyncComplete, Snapshot, EventBatch, Response, ProtocolErrorMessage>;

} // namespace ai::openai::codex::frontend

#endif // AI_OPENAI_CODEX_FRONTEND_MESSAGES_H
