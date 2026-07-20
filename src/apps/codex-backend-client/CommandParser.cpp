/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/CommandParser.h"

#include "ai/openai/codex/frontend/Codec.h"

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
                return usageError("usage: turn <thread-id> <text>");
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
               "  read <thread-id>\n"
               "  turn <thread-id> <text>\n"
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

} // namespace apps::codex_backend_client
