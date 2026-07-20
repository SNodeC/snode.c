/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_STDIO_CLIENT_H
#define AI_OPENAI_CODEX_STDIO_CLIENT_H

#include "ai/openai/codex/AppServerClient.h"

#include <string>
#include <vector>

namespace ai::openai::codex::stdio {

    class Client final : public AppServerClient {
    public:
        Client();
        explicit Client(ClientInfo clientInfo);
        Client(std::string executable, std::vector<std::string> arguments, ClientInfo clientInfo = {});

        ~Client() override;
    };

} // namespace ai::openai::codex::stdio

#endif // AI_OPENAI_CODEX_STDIO_CLIENT_H
