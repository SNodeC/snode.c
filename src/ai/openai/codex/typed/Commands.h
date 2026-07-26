/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_COMMANDS_H
#define AI_OPENAI_CODEX_TYPED_COMMANDS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Types.h"

#include <compare>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ai::openai::codex::typed {

    struct CommandExecProcessId {
        std::string value;

        auto operator<=>(const CommandExecProcessId&) const = default;
    };

    struct CommandExecOutputStream {
        std::string value;

        static CommandExecOutputStream stdoutStream() {
            return {"stdout"};
        }

        static CommandExecOutputStream stderrStream() {
            return {"stderr"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "stdout" || value == "stderr";
        }

        auto operator<=>(const CommandExecOutputStream&) const = default;
    };

    struct CommandExecTerminalSize {
        std::uint16_t cols = 0;
        std::uint16_t rows = 0;

        auto operator<=>(const CommandExecTerminalSize&) const = default;
    };

    struct CommandExecParams {
        std::vector<std::string> command;
        OptionalNullable<std::string> cwd;
        std::optional<bool> disableOutputCap;
        std::optional<bool> disableTimeout;
        OptionalNullable<std::map<std::string, std::optional<std::string>>> env;
        OptionalNullable<std::uint64_t> outputBytesCap;
        OptionalNullable<CommandExecProcessId> processId;
        OptionalNullable<SandboxPolicy> sandboxPolicy;
        OptionalNullable<CommandExecTerminalSize> size;
        std::optional<bool> streamStdin;
        std::optional<bool> streamStdoutStderr;
        OptionalNullable<std::int64_t> timeoutMs;
        std::optional<bool> tty;

        bool operator==(const CommandExecParams&) const = default;
    };

    struct CommandExecResizeParams {
        CommandExecProcessId processId;
        CommandExecTerminalSize size;

        auto operator<=>(const CommandExecResizeParams&) const = default;
    };

    struct CommandExecTerminateParams {
        CommandExecProcessId processId;

        auto operator<=>(const CommandExecTerminateParams&) const = default;
    };

    struct CommandExecWriteParams {
        CommandExecProcessId processId;
        OptionalNullable<std::string> deltaBase64;
        std::optional<bool> closeStdin;

        bool operator==(const CommandExecWriteParams&) const = default;
    };

    struct CommandExecResponse {
        std::int32_t exitCode = 0;
        std::string stdoutData;
        std::string stderrData;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const CommandExecResponse&) const = default;
    };

    struct CommandExecOutputDeltaNotification {
        bool capReached = false;
        std::string deltaBase64;
        CommandExecProcessId processId;
        CommandExecOutputStream stream;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const CommandExecOutputDeltaNotification&) const = default;
    };

    class Commands {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using UnitResultHandler = std::function<void(const OperationResult<Unit>&)>;
        using ExecResult = OperationResult<CommandExecResponse>;
        using ExecResultHandler = std::function<void(const ExecResult&)>;

        Submission exec(CommandExecParams params, ExecResultHandler handler);
        Submission resize(CommandExecResizeParams params, UnitResultHandler handler);
        Submission terminate(CommandExecTerminateParams params, UnitResultHandler handler);
        Submission write(CommandExecWriteParams params, UnitResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Commands(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_COMMANDS_H
