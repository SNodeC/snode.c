/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_SERVERREQUESTS_H
#define AI_OPENAI_CODEX_TYPED_SERVERREQUESTS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Types.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

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
    };

    struct UnknownServerRequest {
        ServerRequestId requestId;
        ServerRequestToken requestToken;
        std::string method;
        Json params;
        Json raw;
        std::optional<std::string> decodingError;
        std::optional<DecodeDiagnostic> diagnostic;
    };

    using TypedServerRequest =
        std::variant<CommandApprovalRequest, FileChangeApprovalRequest, UserInputRequest, AuthenticationRequest, UnknownServerRequest>;

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
        SendResult respond(const FileChangeApprovalRequest& request, ApprovalDecision decision);
        SendResult respond(const UserInputRequest& request, std::vector<UserInputAnswer> answers);
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
