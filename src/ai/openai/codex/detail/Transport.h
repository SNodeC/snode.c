/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_TRANSPORT_H
#define AI_OPENAI_CODEX_DETAIL_TRANSPORT_H

#include "ai/openai/codex/AppServerClient.h"

#include <functional>
#include <string>

namespace ai::openai::codex::detail {

    struct ProcessExit {
        bool exited = false;
        int exitCode = 0;
        bool signaled = false;
        int signalNumber = 0;
    };

    struct TransportCallbacks {
        std::function<void()> onStarted;
        std::function<void(std::string)> onMessage;
        std::function<void(Diagnostic)> onDiagnostic;
        std::function<void(Error)> onError;
        std::function<void(ProcessExit)> onExited;
    };

    class Transport {
    public:
        Transport() = default;
        Transport(const Transport&) = delete;
        Transport(Transport&&) = delete;

        Transport& operator=(const Transport&) = delete;
        Transport& operator=(Transport&&) = delete;

        virtual ~Transport() = default;

        virtual void setCallbacks(TransportCallbacks callbacks) = 0;
        virtual void start() = 0;
        virtual bool send(std::string message) = 0;
        virtual void stop() = 0;
    };

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_TRANSPORT_H
