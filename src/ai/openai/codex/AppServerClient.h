/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_APPSERVERCLIENT_H
#define AI_OPENAI_CODEX_APPSERVERCLIENT_H

#include <functional>
#include <memory>
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

    struct Error {
        enum class Category { Launch, Transport, Protocol, Initialization, Process };

        Category category = Category::Transport;
        int code = 0;
        std::string message;
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
        AppServerClient(const AppServerClient&) = delete;
        AppServerClient(AppServerClient&&) = delete;

        AppServerClient& operator=(const AppServerClient&) = delete;
        AppServerClient& operator=(AppServerClient&&) = delete;

        virtual ~AppServerClient();

        void start();
        void stop();

        State getState() const noexcept;
        bool isReady() const noexcept;

        void setOnStateChanged(Callbacks::StateChanged callback);
        void setOnDiagnostic(Callbacks::DiagnosticReceived callback);

    protected:
        AppServerClient(std::unique_ptr<detail::Transport> transport, ClientInfo clientInfo);

    private:
        class Impl;
        std::unique_ptr<Impl> impl;
    };

} // namespace ai::openai::codex

#endif // AI_OPENAI_CODEX_APPSERVERCLIENT_H
