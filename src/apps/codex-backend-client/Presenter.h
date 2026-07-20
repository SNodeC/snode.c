/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CLIENT_PRESENTER_H
#define APPS_CODEX_BACKEND_CLIENT_PRESENTER_H

#include "ai/openai/codex/frontend/Messages.h"

#include <iosfwd>
#include <string_view>

namespace apps::codex_backend_client {

    enum class OutputMode { Human, Json };

    class Presenter {
    public:
        explicit Presenter(OutputMode mode = OutputMode::Human);
        Presenter(OutputMode mode, std::ostream& output, std::ostream& diagnostics);

        void present(const ai::openai::codex::frontend::ServerMessage& message);

        void setWatchEnabled(bool enabled) noexcept;
        [[nodiscard]] bool watchEnabled() const noexcept;
        [[nodiscard]] OutputMode outputMode() const noexcept;

        void connected(std::string_view socketPath);
        void disconnected();
        void localMessage(std::string_view message);
        void error(std::string_view message);

    private:
        void presentHuman(const ai::openai::codex::frontend::ServerMessage& message);
        void presentJson(const ai::openai::codex::frontend::ServerMessage& message);

        OutputMode mode;
        std::ostream* output;
        std::ostream* diagnostics;
        bool watching = true;
    };

} // namespace apps::codex_backend_client

#endif // APPS_CODEX_BACKEND_CLIENT_PRESENTER_H
