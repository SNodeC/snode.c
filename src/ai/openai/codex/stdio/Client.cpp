/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/stdio/Client.h"

#include "ai/openai/codex/detail/Transport.h"
#include "ai/openai/codex/stdio/StdioTransport.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ai::openai::codex::stdio {

    Client::Client()
        : Client(ClientInfo{}) {
    }

    Client::Client(ClientInfo clientInfo)
        : Client("codex", {"app-server"}, std::move(clientInfo)) {
    }

    Client::Client(std::string executable, std::vector<std::string> arguments, ClientInfo clientInfo)
        : AppServerClient(std::make_unique<detail::StdioTransport>(std::move(executable), std::move(arguments)), std::move(clientInfo)) {
    }

    Client::~Client() = default;

} // namespace ai::openai::codex::stdio
