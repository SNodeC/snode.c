/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_APPSERVERCLIENT_H
#define AI_OPENAI_CODEX_APPSERVERCLIENT_H

#include "ai/openai/codex/Protocol.h"

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace ai::openai::codex::detail {
    class Transport;
}

namespace ai::openai::codex {

    enum class State { Stopped, Starting, Initializing, Ready, Stopping, Failed };

    struct ClientInfo {
        std::string name = "snode_c";
        std::string title = "SNode.C";
        std::string version = "1.0.1";
    };

    struct StateChange {
        State previous = State::Stopped;
        State current = State::Stopped;
        std::optional<Error> error;
    };

    struct Diagnostic {
        std::string message;
    };

    struct Callbacks {
        using StateChanged = std::function<void(const StateChange&)>;
        using DiagnosticReceived = std::function<void(const Diagnostic&)>;

        StateChanged onStateChanged;
        DiagnosticReceived onDiagnostic;
    };

    class AppServerClient {
    public:
        class RawProtocol;

        AppServerClient(const AppServerClient&) = delete;
        AppServerClient(AppServerClient&&) = delete;

        AppServerClient& operator=(const AppServerClient&) = delete;
        AppServerClient& operator=(AppServerClient&&) = delete;

        virtual ~AppServerClient();

        void start();
        void stop();

        State getState() const noexcept;
        bool isReady() const noexcept;

        RawProtocol& raw() noexcept;
        const RawProtocol& raw() const noexcept;

        std::optional<InitializeResult> getInitializeResult() const;

        void setOnStateChanged(Callbacks::StateChanged callback);
        void setOnDiagnostic(Callbacks::DiagnosticReceived callback);

    protected:
        AppServerClient(std::unique_ptr<detail::Transport> transport, ClientInfo clientInfo);

    private:
        class Impl;
        std::unique_ptr<Impl> impl;
    };

    class AppServerClient::RawProtocol {
    public:
        using ResponseHandler = std::function<void(const Response&)>;
        using NotificationHandler = std::function<void(const Notification&)>;
        using ServerRequestHandler = std::function<void(const ServerRequest&)>;
        using UnknownMessageHandler = std::function<void(const UnknownMessage&)>;

        struct Submission {
            std::optional<ClientRequestId> id;
            std::optional<Error> error;

            explicit operator bool() const noexcept;
        };

        struct SendResult {
            bool accepted = false;
            std::optional<Error> error;

            explicit operator bool() const noexcept;
        };

        Submission request(std::string method, Json params, ResponseHandler handler);
        SendResult notify(std::string method, Json params = Json::object());
        SendResult respond(const ServerRequestId& id, Json result);
        SendResult reject(const ServerRequestId& id, ProtocolError error);

        void setOnNotification(NotificationHandler handler);
        void setOnServerRequest(ServerRequestHandler handler);
        void setOnUnknownMessage(UnknownMessageHandler handler);

    private:
        friend class AppServerClient::Impl;

        explicit RawProtocol(Impl& impl) noexcept;

        Impl* impl;
    };

} // namespace ai::openai::codex

#endif // AI_OPENAI_CODEX_APPSERVERCLIENT_H
