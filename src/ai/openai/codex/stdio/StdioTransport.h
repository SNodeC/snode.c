/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_STDIO_STDIOTRANSPORT_H
#define AI_OPENAI_CODEX_STDIO_STDIOTRANSPORT_H

#include "ai/openai/codex/detail/Transport.h"

#include <memory>
#include <string>
#include <vector>

namespace ai::openai::codex::stdio::detail {

    class StdioTransport final : public codex::detail::Transport {
    public:
        class Session;

        StdioTransport(std::string executable,
                       std::vector<std::string> arguments,
                       bool forceChildExitPollingForTests = false,
                       bool failParentSetupForTests = false);
        ~StdioTransport() override;

        void setCallbacks(codex::detail::TransportCallbacks callbacks) override;
        void start() override;
        bool send(std::string message) override;
        void stop() override;

    private:
        std::string executable;
        std::vector<std::string> arguments;
        bool forceChildExitPollingForTests;
        bool failParentSetupForTests;
        codex::detail::TransportCallbacks callbacks;
        std::shared_ptr<Session> session;
    };

} // namespace ai::openai::codex::stdio::detail

#endif // AI_OPENAI_CODEX_STDIO_STDIOTRANSPORT_H
