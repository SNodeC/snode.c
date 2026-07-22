/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CLIENT_COMMANDPARSER_H
#define APPS_CODEX_BACKEND_CLIENT_COMMANDPARSER_H

#include "ai/openai/codex/frontend/Messages.h"

#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace apps::codex_backend_client {

    struct NoopCommand {
        bool operator==(const NoopCommand&) const = default;
    };

    struct HelpCommand {
        bool operator==(const HelpCommand&) const = default;
    };

    struct QuitCommand {
        bool operator==(const QuitCommand&) const = default;
    };

    struct WatchCommand {
        bool enabled = true;

        bool operator==(const WatchCommand&) const = default;
    };

    struct SendCommand {
        ai::openai::codex::frontend::ClientMessage message;

        bool operator==(const SendCommand&) const = default;
    };

    struct NewCommand {
        std::string threadStartRequestId;
        std::string turnStartRequestId;
        ai::openai::codex::frontend::ThreadStart options;
        std::string prompt;

        bool operator==(const NewCommand&) const = default;
    };

    struct CommandParseError {
        std::string message;

        bool operator==(const CommandParseError&) const = default;
    };

    using ParsedCommand = std::variant<NoopCommand, HelpCommand, QuitCommand, WatchCommand, SendCommand, NewCommand, CommandParseError>;

    class CommandParser {
    public:
        explicit CommandParser(std::string requestIdPrefix = "client");

        [[nodiscard]] ParsedCommand parse(std::string_view line);
        [[nodiscard]] static std::string helpText();

    private:
        [[nodiscard]] std::string allocateRequestId();
        [[nodiscard]] std::optional<std::pair<std::string, std::string>> allocateRequestIdPair();

        std::string requestIdPrefix;
        std::uint64_t nextRequestId = 1;
        bool requestIdsExhausted = false;
    };

} // namespace apps::codex_backend_client

#endif // APPS_CODEX_BACKEND_CLIENT_COMMANDPARSER_H
