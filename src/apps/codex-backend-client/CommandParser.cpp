/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/CommandParser.h"

#include "ai/openai/codex/frontend/Codec.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

namespace apps::codex_backend_client {

    namespace {

        namespace frontend = ai::openai::codex::frontend;

        bool isSpace(char value) noexcept {
            return value == ' ' || value == '\t' || value == '\r' || value == '\n' || value == '\f' || value == '\v';
        }

        std::string_view trim(std::string_view value) noexcept {
            while (!value.empty() && isSpace(value.front())) {
                value.remove_prefix(1);
            }
            while (!value.empty() && isSpace(value.back())) {
                value.remove_suffix(1);
            }
            return value;
        }

        std::string_view takeWord(std::string_view& value) noexcept {
            value = trim(value);
            std::size_t length = 0;
            while (length < value.size() && !isSpace(value[length])) {
                ++length;
            }
            const std::string_view word = value.substr(0, length);
            value.remove_prefix(length);
            value = trim(value);
            return word;
        }

        std::optional<std::uint64_t> parseSequence(std::string_view value) noexcept {
            value = trim(value);
            if (value.empty()) {
                return std::nullopt;
            }

            std::uint64_t sequence = 0;
            const char* const begin = value.data();
            const char* const end = begin + value.size();
            const auto conversion = std::from_chars(begin, end, sequence, 10);
            if (conversion.ec != std::errc{} || conversion.ptr != end) {
                return std::nullopt;
            }
            return sequence;
        }

        CommandParseError usageError(std::string message) {
            return CommandParseError{std::move(message)};
        }

        bool parseThreadOption(std::string_view option,
                               std::string_view& remainder,
                               frontend::ThreadStart& parsed,
                               bool allowEphemeral,
                               std::vector<std::string_view>& seen,
                               std::string& error) {
            const bool known = option == "--cwd" || option == "--model" || option == "--model-provider" || option == "--approval-policy" ||
                               option == "--sandbox-mode" || option == "--ephemeral";
            if (!known) {
                error = "unknown option '" + std::string(option) + "'";
                return false;
            }
            if (option == "--ephemeral" && !allowEphemeral) {
                error = "option '--ephemeral' is not supported by resume";
                return false;
            }
            if (std::find(seen.begin(), seen.end(), option) != seen.end()) {
                error = "option '" + std::string(option) + "' may only be specified once";
                return false;
            }
            seen.push_back(option);

            if (option == "--ephemeral") {
                parsed.ephemeral = true;
                return true;
            }

            const std::string_view value = takeWord(remainder);
            if (value.empty() || value.starts_with("--")) {
                error = "option '" + std::string(option) + "' requires a value";
                return false;
            }

            if (option == "--cwd") {
                parsed.cwd = std::string(value);
            } else if (option == "--model") {
                parsed.model = std::string(value);
            } else if (option == "--model-provider") {
                parsed.modelProvider = std::string(value);
            } else if (option == "--approval-policy") {
                parsed.approvalPolicy = std::string(value);
            } else {
                parsed.sandboxMode = std::string(value);
            }
            return true;
        }

        bool parseThreadOptions(std::string_view& remainder,
                                frontend::ThreadStart& parsed,
                                bool allowEphemeral,
                                bool stopAtSeparator,
                                bool& separatorSeen,
                                std::string& error) {
            std::vector<std::string_view> seen;
            while (!remainder.empty()) {
                const std::string_view option = takeWord(remainder);
                if (option == "--") {
                    if (stopAtSeparator) {
                        separatorSeen = true;
                        return true;
                    }
                    error = "unexpected '--' separator";
                    return false;
                }
                if (!option.starts_with("--")) {
                    error = "unexpected positional argument '" + std::string(option) + "'";
                    return false;
                }
                if (!parseThreadOption(option, remainder, parsed, allowEphemeral, seen, error)) {
                    return false;
                }
            }
            return true;
        }

        std::string commandUsage(std::string_view command, std::string detail) {
            return std::string(command) + ": " + std::move(detail) + "; enter 'help' for command syntax";
        }

    } // namespace

    CommandParser::CommandParser(std::string requestIdPrefix)
        : requestIdPrefix(requestIdPrefix.empty() ? "client" : std::move(requestIdPrefix)) {
    }

    ParsedCommand CommandParser::parse(std::string_view line) {
        line = trim(line);
        if (line.empty()) {
            return NoopCommand{};
        }

        std::string_view remainder = line;
        const std::string_view name = takeWord(remainder);

        if (name == "help") {
            return remainder.empty() ? ParsedCommand{HelpCommand{}} : ParsedCommand{usageError("usage: help")};
        }
        if (name == "quit") {
            return remainder.empty() ? ParsedCommand{QuitCommand{}} : ParsedCommand{usageError("usage: quit")};
        }
        if (name == "watch") {
            const std::string_view value = takeWord(remainder);
            if (!remainder.empty() || (value != "on" && value != "off")) {
                return usageError("usage: watch on|off");
            }
            return WatchCommand{value == "on"};
        }

        const auto send = [this](frontend::CommandParameters parameters) -> ParsedCommand {
            std::string requestId = allocateRequestId();
            if (requestId.empty()) {
                return usageError("frontend request ID space is exhausted");
            }
            return SendCommand{
                frontend::Command{std::move(requestId), std::move(parameters), frontend::Json::object(), frontend::Json::object()}};
        };

        if (name == "snapshot") {
            return remainder.empty() ? send(frontend::SnapshotGet{}) : ParsedCommand{usageError("usage: snapshot")};
        }
        if (name == "acquire") {
            return remainder.empty() ? send(frontend::ControllerAcquire{}) : ParsedCommand{usageError("usage: acquire")};
        }
        if (name == "release") {
            return remainder.empty() ? send(frontend::ControllerRelease{}) : ParsedCommand{usageError("usage: release")};
        }
        if (name == "threads") {
            return remainder.empty() ? send(frontend::ThreadList{}) : ParsedCommand{usageError("usage: threads")};
        }
        if (name == "start") {
            frontend::ThreadStart start;
            bool separatorSeen = false;
            std::string error;
            if (!parseThreadOptions(remainder, start, true, false, separatorSeen, error)) {
                return usageError(commandUsage(name, std::move(error)));
            }
            return send(std::move(start));
        }
        if (name == "resume") {
            const std::string_view threadId = takeWord(remainder);
            if (threadId.empty() || threadId.starts_with("--")) {
                return usageError("usage: resume <thread-id> [thread-resume-options]");
            }

            frontend::ThreadStart options;
            bool separatorSeen = false;
            std::string error;
            if (!parseThreadOptions(remainder, options, false, false, separatorSeen, error)) {
                return usageError(commandUsage(name, std::move(error)));
            }

            frontend::ThreadResume resume;
            resume.threadId = std::string(threadId);
            resume.cwd = std::move(options.cwd);
            resume.model = std::move(options.model);
            resume.modelProvider = std::move(options.modelProvider);
            resume.approvalPolicy = std::move(options.approvalPolicy);
            resume.sandboxMode = std::move(options.sandboxMode);
            return send(std::move(resume));
        }
        if (name == "new") {
            if (remainder.empty()) {
                return usageError("usage: new [thread-start-options] -- <prompt> | new <prompt>");
            }

            frontend::ThreadStart options;
            std::string prompt;
            std::string_view probe = remainder;
            const std::string_view first = takeWord(probe);
            if (!first.starts_with("--")) {
                prompt = std::string(remainder);
            } else {
                bool separatorSeen = false;
                std::string error;
                if (!parseThreadOptions(remainder, options, true, true, separatorSeen, error)) {
                    return usageError(commandUsage(name, std::move(error)));
                }
                if (!separatorSeen) {
                    return usageError("new: thread-start options must be followed by '-- <prompt>'; enter 'help' for command syntax");
                }
                const std::string_view text = trim(remainder);
                if (text.empty()) {
                    return usageError("new: prompt must not be empty; enter 'help' for command syntax");
                }
                prompt = std::string(text);
            }

            const auto requestIds = allocateRequestIdPair();
            if (!requestIds.has_value()) {
                return usageError("frontend request ID space cannot allocate two IDs for new");
            }
            return NewCommand{requestIds->first, requestIds->second, std::move(options), std::move(prompt)};
        }
        if (name == "replay") {
            const std::string_view value = takeWord(remainder);
            const std::optional<std::uint64_t> sequence = parseSequence(value);
            if (!remainder.empty() || !sequence.has_value()) {
                return usageError("usage: replay <sequence>");
            }
            return send(frontend::ReplayAfter{frontend::SequenceNumber(*sequence)});
        }
        if (name == "read") {
            const std::string_view threadId = takeWord(remainder);
            if (threadId.empty() || !remainder.empty()) {
                return usageError("usage: read <thread-id>");
            }
            return send(frontend::ThreadRead{std::string(threadId), std::nullopt});
        }
        if (name == "turn") {
            const std::string_view threadId = takeWord(remainder);
            const std::string_view text = trim(remainder);
            if (threadId.empty() || text.empty()) {
                return usageError("usage: turn <thread-id> <prompt>");
            }

            frontend::TurnStart turn;
            turn.threadId = std::string(threadId);
            turn.input.emplace_back(frontend::TextInput{std::string(text), frontend::Json::object()});
            return send(std::move(turn));
        }
        if (name == "interrupt") {
            const std::string_view threadId = takeWord(remainder);
            const std::string_view turnId = takeWord(remainder);
            if (threadId.empty() || turnId.empty() || !remainder.empty()) {
                return usageError("usage: interrupt <thread-id> <turn-id>");
            }
            return send(frontend::TurnInterrupt{std::string(threadId), std::string(turnId)});
        }
        if (name == "raw") {
            if (remainder.empty()) {
                return usageError("usage: raw <json>");
            }

            frontend::Json encoded = frontend::Json::parse(remainder.begin(), remainder.end(), nullptr, false);
            if (encoded.is_discarded()) {
                return usageError("raw frontend message is not valid JSON");
            }
            auto decoded = frontend::Codec::decodeClient(encoded);
            if (!decoded) {
                return usageError("raw frontend message rejected: " + decoded.error().message);
            }
            return SendCommand{std::move(decoded).value()};
        }

        return usageError("unknown command '" + std::string(name) + "'; enter 'help' for available commands");
    }

    std::string CommandParser::helpText() {
        return "Commands:\n"
               "  help\n"
               "  quit\n"
               "  snapshot\n"
               "  replay <sequence>\n"
               "  acquire\n"
               "  release\n"
               "  threads\n"
               "  start [--cwd <path>] [--model <model>]\n"
               "        [--model-provider <provider>]\n"
               "        [--approval-policy <policy>]\n"
               "        [--sandbox-mode <mode>]\n"
               "        [--ephemeral]\n"
               "  resume <thread-id>\n"
               "         [--cwd <path>] [--model <model>]\n"
               "         [--model-provider <provider>]\n"
               "         [--approval-policy <policy>]\n"
               "         [--sandbox-mode <mode>]\n"
               "  new [thread-start-options] -- <prompt>\n"
               "  new <prompt>\n"
               "  read <thread-id>\n"
               "  turn <thread-id> <prompt>\n"
               "  interrupt <thread-id> <turn-id>\n"
               "  raw <json>\n"
               "  watch on\n"
               "  watch off";
    }

    std::string CommandParser::allocateRequestId() {
        if (requestIdsExhausted) {
            return {};
        }

        const std::string requestId = requestIdPrefix + "-" + std::to_string(nextRequestId);
        if (nextRequestId == std::numeric_limits<std::uint64_t>::max()) {
            requestIdsExhausted = true;
        } else {
            ++nextRequestId;
        }
        return requestId;
    }

    std::optional<std::pair<std::string, std::string>> CommandParser::allocateRequestIdPair() {
        if (requestIdsExhausted || nextRequestId == std::numeric_limits<std::uint64_t>::max()) {
            return std::nullopt;
        }

        const std::uint64_t firstSequence = nextRequestId;
        const std::uint64_t secondSequence = firstSequence + 1;
        std::pair<std::string, std::string> requestIds{requestIdPrefix + "-" + std::to_string(firstSequence),
                                                       requestIdPrefix + "-" + std::to_string(secondSequence)};
        if (secondSequence == std::numeric_limits<std::uint64_t>::max()) {
            requestIdsExhausted = true;
        } else {
            nextRequestId = secondSequence + 1;
        }
        return requestIds;
    }

} // namespace apps::codex_backend_client
