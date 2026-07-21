/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/frontend/Messages.h"
#include "ai/openai/codex/frontend/Protocol.h"
#include "apps/codex-backend-client/CommandParser.h"
#include "support/TestResult.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace client = apps::codex_backend_client;
    namespace frontend = ai::openai::codex::frontend;

    const client::SendCommand* sendCommand(const client::ParsedCommand& parsed) {
        return std::get_if<client::SendCommand>(&parsed);
    }

    const client::NewCommand* newCommand(const client::ParsedCommand& parsed) {
        return std::get_if<client::NewCommand>(&parsed);
    }

    const frontend::Command* protocolCommand(const client::ParsedCommand& parsed) {
        const client::SendCommand* send = sendCommand(parsed);
        return send == nullptr ? nullptr : std::get_if<frontend::Command>(&send->message);
    }

    template <typename ParametersT>
    const ParametersT* parameters(const client::ParsedCommand& parsed) {
        const frontend::Command* command = protocolCommand(parsed);
        return command == nullptr ? nullptr : std::get_if<ParametersT>(&command->parameters);
    }

    void expectWireCommand(tests::support::TestResult& result,
                           const client::ParsedCommand& parsed,
                           std::string_view method,
                           frontend::Json params,
                           const std::string& description) {
        const client::SendCommand* send = sendCommand(parsed);
        const frontend::Command* command = protocolCommand(parsed);
        result.expectTrue(send != nullptr && command != nullptr, description + " produces a protocol command");
        if (send == nullptr || command == nullptr) {
            return;
        }

        const auto encoded = frontend::Codec::encodeClient(send->message);
        result.expectTrue(encoded.hasValue(), description + " encodes through the shared frontend codec");
        if (!encoded) {
            return;
        }

        const frontend::Json expected{{"protocol", frontend::ProtocolIdentity},
                                      {"version", frontend::ProtocolVersion},
                                      {"kind", frontend::kind::Command},
                                      {"requestId", command->requestId},
                                      {"method", method},
                                      {"params", std::move(params)}};
        result.expectTrue(!command->requestId.empty() && encoded.value() == expected,
                          description + " uses the exact Frontend Protocol v1 envelope and parameters");
    }

    void expectWireCommand(tests::support::TestResult& result,
                           const frontend::Command& command,
                           std::string_view method,
                           frontend::Json params,
                           const std::string& description) {
        const auto encoded = frontend::Codec::encodeClient(frontend::ClientMessage{command});
        result.expectTrue(encoded.hasValue(), description + " encodes through the shared frontend codec");
        if (!encoded) {
            return;
        }

        const frontend::Json expected{{"protocol", frontend::ProtocolIdentity},
                                      {"version", frontend::ProtocolVersion},
                                      {"kind", frontend::kind::Command},
                                      {"requestId", command.requestId},
                                      {"method", method},
                                      {"params", std::move(params)}};
        result.expectTrue(!command.requestId.empty() && encoded.value() == expected,
                          description + " uses the exact Frontend Protocol v1 envelope and parameters");
    }

    bool hasNoStartOptions(const frontend::ThreadStart& start) {
        return !start.cwd.has_value() && !start.model.has_value() && !start.modelProvider.has_value() &&
               !start.approvalPolicy.has_value() && !start.sandboxMode.has_value() && !start.ephemeral.has_value();
    }

    const client::CommandParseError* parseError(const client::ParsedCommand& parsed) {
        return std::get_if<client::CommandParseError>(&parsed);
    }
} // namespace

int main() {
    using namespace apps::codex_backend_client;
    using namespace ai::openai::codex::frontend;

    tests::support::TestResult result;
    CommandParser parser;

    const ParsedCommand empty = parser.parse("  \t ");
    result.expectTrue(std::holds_alternative<NoopCommand>(empty), "blank input is a local no-op");
    result.expectTrue(std::holds_alternative<HelpCommand>(parser.parse("help")), "help remains a local presentation command");
    result.expectTrue(std::holds_alternative<QuitCommand>(parser.parse("quit")), "quit remains a local lifecycle command");

    const ParsedCommand watchOn = parser.parse("watch on");
    const ParsedCommand watchOff = parser.parse("watch off");
    const WatchCommand* enabled = std::get_if<WatchCommand>(&watchOn);
    const WatchCommand* disabled = std::get_if<WatchCommand>(&watchOff);
    result.expectTrue(enabled != nullptr && enabled->enabled, "watch on enables event presentation locally");
    result.expectTrue(disabled != nullptr && !disabled->enabled, "watch off disables event presentation locally");

    const ParsedCommand snapshot = parser.parse("snapshot");
    result.expectTrue(parameters<SnapshotGet>(snapshot) != nullptr, "snapshot maps to typed snapshot.get parameters");
    result.expectTrue(protocolCommand(snapshot) != nullptr && protocolCommand(snapshot)->requestId == "client-1",
                      "the first protocol command receives the documented monotonic client request ID");
    expectWireCommand(result, snapshot, method::SnapshotGet, Json::object(), "snapshot");

    const ParsedCommand replay = parser.parse("replay 42");
    const ReplayAfter* replayParameters = parameters<ReplayAfter>(replay);
    result.expectTrue(replayParameters != nullptr && replayParameters->after == SequenceNumber{42},
                      "replay parses the exact unsigned sequence number");
    result.expectTrue(protocolCommand(replay) != nullptr && protocolCommand(replay)->requestId == "client-2",
                      "the next protocol command advances the request ID exactly once");
    expectWireCommand(result, replay, method::EventsReplay, Json{{"after", std::uint64_t{42}}}, "replay");

    const ParsedCommand acquire = parser.parse("acquire");
    const ParsedCommand release = parser.parse("release");
    result.expectTrue(parameters<ControllerAcquire>(acquire) != nullptr, "acquire maps to controller.acquire");
    result.expectTrue(parameters<ControllerRelease>(release) != nullptr, "release maps to controller.release");
    expectWireCommand(result, acquire, method::ControllerAcquire, Json::object(), "acquire");
    expectWireCommand(result, release, method::ControllerRelease, Json::object(), "release");

    const Command* acquireCommand = protocolCommand(acquire);
    const Command* releaseCommand = protocolCommand(release);
    result.expectTrue(acquireCommand != nullptr && releaseCommand != nullptr && acquireCommand->requestId != releaseCommand->requestId,
                      "generated request identifiers are non-empty and unique");

    const ParsedCommand threads = parser.parse("threads");
    result.expectTrue(parameters<ThreadList>(threads) != nullptr, "threads maps to a typed thread.list command");
    expectWireCommand(result, threads, method::ThreadList, Json::object(), "threads");

    CommandParser threadParser("thread-command");

    const ParsedCommand bareStart = threadParser.parse("start");
    const ThreadStart* bareStartParameters = parameters<ThreadStart>(bareStart);
    result.expectTrue(bareStartParameters != nullptr && hasNoStartOptions(*bareStartParameters),
                      "bare start leaves every optional Codex thread.start setting unset");
    expectWireCommand(result, bareStart, method::ThreadStart, Json::object(), "bare start");

    const ParsedCommand configuredStart =
        threadParser.parse("start --cwd /tmp/project --model gpt-5 --model-provider openai --approval-policy on-request "
                           "--sandbox-mode workspace-write --ephemeral");
    const ThreadStart* configuredStartParameters = parameters<ThreadStart>(configuredStart);
    result.expectTrue(configuredStartParameters != nullptr && configuredStartParameters->cwd == "/tmp/project" &&
                          configuredStartParameters->model == "gpt-5" && configuredStartParameters->modelProvider == "openai" &&
                          configuredStartParameters->approvalPolicy == "on-request" &&
                          configuredStartParameters->sandboxMode == "workspace-write" && configuredStartParameters->ephemeral == true,
                      "start maps every supported option to the corresponding typed optional field");
    expectWireCommand(result,
                      configuredStart,
                      method::ThreadStart,
                      Json{{"cwd", "/tmp/project"},
                           {"model", "gpt-5"},
                           {"modelProvider", "openai"},
                           {"approvalPolicy", "on-request"},
                           {"sandboxMode", "workspace-write"},
                           {"ephemeral", true}},
                      "configured start");

    const ParsedCommand bareResume = threadParser.parse("resume thread-existing");
    const ThreadResume* bareResumeParameters = parameters<ThreadResume>(bareResume);
    result.expectTrue(bareResumeParameters != nullptr && bareResumeParameters->threadId == "thread-existing" &&
                          !bareResumeParameters->cwd.has_value() && !bareResumeParameters->model.has_value() &&
                          !bareResumeParameters->modelProvider.has_value() && !bareResumeParameters->approvalPolicy.has_value() &&
                          !bareResumeParameters->sandboxMode.has_value(),
                      "bare resume preserves its thread ID and leaves all optional resume settings unset");
    expectWireCommand(result, bareResume, method::ThreadResume, Json{{"threadId", "thread-existing"}}, "bare resume");

    const ParsedCommand configuredResume =
        threadParser.parse("resume thread-existing --cwd /tmp/resumed --model gpt-5-codex --model-provider azure "
                           "--approval-policy never --sandbox-mode read-only");
    const ThreadResume* configuredResumeParameters = parameters<ThreadResume>(configuredResume);
    result.expectTrue(configuredResumeParameters != nullptr && configuredResumeParameters->threadId == "thread-existing" &&
                          configuredResumeParameters->cwd == "/tmp/resumed" && configuredResumeParameters->model == "gpt-5-codex" &&
                          configuredResumeParameters->modelProvider == "azure" && configuredResumeParameters->approvalPolicy == "never" &&
                          configuredResumeParameters->sandboxMode == "read-only",
                      "resume maps every supported option while retaining the explicit persisted thread ID");
    expectWireCommand(result,
                      configuredResume,
                      method::ThreadResume,
                      Json{{"threadId", "thread-existing"},
                           {"cwd", "/tmp/resumed"},
                           {"model", "gpt-5-codex"},
                           {"modelProvider", "azure"},
                           {"approvalPolicy", "never"},
                           {"sandboxMode", "read-only"}},
                      "configured resume");

    const ParsedCommand simpleNew = threadParser.parse("new Explain the  current repository architecture.");
    const NewCommand* simpleNewCommand = newCommand(simpleNew);
    result.expectTrue(
        simpleNewCommand != nullptr && !simpleNewCommand->threadStartRequestId.empty() && !simpleNewCommand->turnStartRequestId.empty() &&
            simpleNewCommand->threadStartRequestId != simpleNewCommand->turnStartRequestId &&
            hasNoStartOptions(simpleNewCommand->options) && simpleNewCommand->prompt == "Explain the  current repository architecture.",
        "simple new carries two distinct generated IDs, unset start options, and the exact line-oriented prompt");
    if (simpleNewCommand != nullptr) {
        expectWireCommand(result,
                          Command{simpleNewCommand->threadStartRequestId, simpleNewCommand->options, Json::object(), Json::object()},
                          method::ThreadStart,
                          Json::object(),
                          "simple new internal start");
    }

    const ParsedCommand configuredNew =
        threadParser.parse("new --cwd /tmp/new --model gpt-5 --model-provider openai --approval-policy on-request "
                           "--sandbox-mode workspace-write --ephemeral -- Review the current implementation. --include-tests");
    const NewCommand* configuredNewCommand = newCommand(configuredNew);
    result.expectTrue(
        configuredNewCommand != nullptr && configuredNewCommand->threadStartRequestId != configuredNewCommand->turnStartRequestId &&
            configuredNewCommand->options.cwd == "/tmp/new" && configuredNewCommand->options.model == "gpt-5" &&
            configuredNewCommand->options.modelProvider == "openai" && configuredNewCommand->options.approvalPolicy == "on-request" &&
            configuredNewCommand->options.sandboxMode == "workspace-write" && configuredNewCommand->options.ephemeral == true &&
            configuredNewCommand->prompt == "Review the current implementation. --include-tests",
        "configured new keeps every start option and preserves words beginning with '--' after the separator");
    if (configuredNewCommand != nullptr) {
        expectWireCommand(
            result,
            Command{configuredNewCommand->threadStartRequestId, configuredNewCommand->options, Json::object(), Json::object()},
            method::ThreadStart,
            Json{{"cwd", "/tmp/new"},
                 {"model", "gpt-5"},
                 {"modelProvider", "openai"},
                 {"approvalPolicy", "on-request"},
                 {"sandboxMode", "workspace-write"},
                 {"ephemeral", true}},
            "configured new internal start");
    }

    const ParsedCommand separatedNew = threadParser.parse("new -- Prompt beginning with --literal and containing spaces");
    const NewCommand* separatedNewCommand = newCommand(separatedNew);
    result.expectTrue(separatedNewCommand != nullptr && hasNoStartOptions(separatedNewCommand->options) &&
                          separatedNewCommand->prompt == "Prompt beginning with --literal and containing spaces",
                      "new accepts an explicit separator without options and retains the complete prompt after it");

    const ParsedCommand read = parser.parse("read thread-7");
    const ThreadRead* readParameters = parameters<ThreadRead>(read);
    result.expectTrue(readParameters != nullptr && readParameters->threadId == "thread-7" && readParameters->includeTurns == std::nullopt,
                      "read preserves the thread identifier and leaves the optional includeTurns policy unspecified");
    expectWireCommand(result, read, method::ThreadRead, Json{{"threadId", "thread-7"}}, "read");

    const ParsedCommand turn = parser.parse("turn thread-7 summarize these changes");
    const TurnStart* turnParameters = parameters<TurnStart>(turn);
    const TextInput* text =
        turnParameters != nullptr && turnParameters->input.size() == 1 ? std::get_if<TextInput>(&turnParameters->input.front()) : nullptr;
    result.expectTrue(turnParameters != nullptr && turnParameters->threadId == "thread-7" && text != nullptr &&
                          text->text == "summarize these changes",
                      "turn keeps the complete line-oriented text as one typed text input");
    expectWireCommand(result,
                      turn,
                      method::TurnStart,
                      Json{{"threadId", "thread-7"}, {"input", Json::array({Json{{"type", "text"}, {"text", "summarize these changes"}}})}},
                      "turn");

    const ParsedCommand interrupt = parser.parse("interrupt thread-7 turn-9");
    const TurnInterrupt* interruptParameters = parameters<TurnInterrupt>(interrupt);
    result.expectTrue(interruptParameters != nullptr && interruptParameters->threadId == "thread-7" &&
                          interruptParameters->turnId == "turn-9",
                      "interrupt maps both identifiers without changing their spelling");
    expectWireCommand(result, interrupt, method::TurnInterrupt, Json{{"threadId", "thread-7"}, {"turnId", "turn-9"}}, "interrupt");

    const std::string rawJson =
        R"({"protocol":"snodec.codex-frontend","version":1,"kind":"command","requestId":"raw-1","method":"snapshot.get","params":{}})";
    const ParsedCommand raw = parser.parse("raw " + rawJson);
    const SendCommand* rawSend = sendCommand(raw);
    bool rawRoundTrips = false;
    if (rawSend != nullptr) {
        const auto rawEncoded = Codec::encodeClient(rawSend->message);
        rawRoundTrips = rawEncoded.hasValue() && rawEncoded.value() == Json::parse(rawJson);
    }
    result.expectTrue(rawRoundTrips,
                      "raw accepts only a valid frontend message and reuses the shared codec without changing its wire fields");

    CommandParser validationParser("validation");
    const std::vector<std::pair<std::string_view, std::string_view>> invalidThreadCommands{
        {"start unexpected", "unexpected positional argument"},
        {"start --cwd", "requires a value"},
        {"start --cwd --model gpt-5", "requires a value"},
        {"start --unknown value", "unknown option"},
        {"start --model first --model second", "may only be specified once"},
        {"start --ephemeral --ephemeral", "may only be specified once"},
        {"resume", "usage: resume"},
        {"resume --cwd /tmp/project", "usage: resume"},
        {"resume thread-1 unexpected", "unexpected positional argument"},
        {"resume thread-1 --cwd", "requires a value"},
        {"resume thread-1 --unknown value", "unknown option"},
        {"resume thread-1 --model first --model second", "may only be specified once"},
        {"resume thread-1 --ephemeral", "not supported by resume"},
        {"new", "usage: new"},
        {"new --cwd", "requires a value"},
        {"new --", "prompt must not be empty"},
        {"new --cwd /tmp/project --", "prompt must not be empty"},
        {"new --cwd /tmp/project missing-separator", "unexpected positional argument"},
        {"new --cwd /tmp/project", "must be followed by '-- <prompt>'"},
        {"new --unknown value -- prompt", "unknown option"},
        {"new --model first --model second -- prompt", "may only be specified once"},
        {"new --ephemeral --ephemeral -- prompt", "may only be specified once"},
    };
    for (const auto& [line, expectedDiagnostic] : invalidThreadCommands) {
        const ParsedCommand invalid = validationParser.parse(line);
        const CommandParseError* error = parseError(invalid);
        result.expectTrue(error != nullptr && error->message.find(expectedDiagnostic) != std::string::npos,
                          "invalid thread command reports a clear local error: " + std::string(line));
    }

    const ParsedCommand firstAfterValidationErrors = validationParser.parse("start");
    result.expectTrue(protocolCommand(firstAfterValidationErrors) != nullptr &&
                          protocolCommand(firstAfterValidationErrors)->requestId == "validation-1",
                      "invalid start, resume, and new inputs do not consume generated frontend request IDs");

    for (const std::string_view invalid :
         {"replay nope", "read", "turn thread-7", "interrupt thread-7", "watch maybe", "raw {not-json", "unknown-command"}) {
        result.expectTrue(std::holds_alternative<CommandParseError>(parser.parse(invalid)),
                          std::string("invalid command is rejected locally: ") + std::string(invalid));
    }

    const std::string help = CommandParser::helpText();
    for (const std::string_view command : {"snapshot",
                                           "replay",
                                           "acquire",
                                           "release",
                                           "threads",
                                           "start",
                                           "resume",
                                           "new",
                                           "read",
                                           "turn",
                                           "interrupt",
                                           "raw",
                                           "watch",
                                           "quit"}) {
        result.expectTrue(help.find(command) != std::string::npos, "help documents command: " + std::string(command));
    }
    for (const std::string_view option :
         {"--cwd", "--model", "--model-provider", "--approval-policy", "--sandbox-mode", "--ephemeral", "-- <prompt>"}) {
        result.expectTrue(help.find(option) != std::string::npos, "help documents thread option syntax: " + std::string(option));
    }

    return result.processResult();
}
