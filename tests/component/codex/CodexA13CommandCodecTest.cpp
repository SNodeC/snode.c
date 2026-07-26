/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ClientOperationCodec.h"
#include "ai/openai/codex/detail/CommandCodec.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Commands.h"
#include "ai/openai/codex/typed/Events.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    bool hasDiagnostic(const std::vector<typed::DecodeDiagnostic>& diagnostics,
                       typed::DecodeIssueKind kind,
                       typed::DecodeIssueSeverity severity,
                       std::string_view surface,
                       std::string_view path) {
        for (const typed::DecodeDiagnostic& diagnostic : diagnostics) {
            if (diagnostic.kind == kind && diagnostic.severity == severity && diagnostic.surface == surface &&
                diagnostic.fieldPath == path) {
                return true;
            }
        }
        return false;
    }

    codex::Notification outputNotification(codex::Json params) {
        const codex::Json raw{
            {"jsonrpc", "2.0"},
            {"method", "command/exec/outputDelta"},
            {"params", params},
        };
        return {"command/exec/outputDelta", std::move(params), raw};
    }

    void testOpenStreamValues(tests::support::TestResult& result) {
        result.expectTrue(typed::CommandExecOutputStream::stdoutStream().value == "stdout" &&
                              typed::CommandExecOutputStream::stdoutStream().isKnown() &&
                              typed::CommandExecOutputStream::stderrStream().value == "stderr" &&
                              typed::CommandExecOutputStream::stderrStream().isKnown(),
                          "command output stream factories preserve both pinned stable values");
        result.expectTrue(!typed::CommandExecOutputStream{"future-stream"}.isKnown(),
                          "command output stream remains open to future wire values");
    }

    void testExecEncoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const auto minimum = detail::encodeCommandExecParams({.command = {"synthetic-command"}}, error);
        result.expectTrue(minimum == codex::Json{{"command", codex::Json::array({"synthetic-command"})}} && error.empty(),
                          "command/exec minimum request emits only its required argv vector");

        const auto empty = detail::encodeCommandExecParams({}, error);
        result.expectTrue(!empty && error.find("$.command") != std::string::npos && error.find("synthetic-command") == std::string::npos,
                          "command/exec rejects an empty argv array with a structural, redacted diagnostic");

        const typed::WorkspaceWriteSandboxPolicy workspaceWrite{
            .writableRoots =
                std::vector<typed::AbsolutePathBuf>{
                    typed::AbsolutePathBuf{"/synthetic/root-a"},
                    typed::AbsolutePathBuf{"/synthetic/root-b"},
                },
            .networkAccess = false,
            .excludeTmpdirEnvVar = true,
            .excludeSlashTmp = false,
        };
        const typed::CommandExecParams full{
            .command =
                {
                    "synthetic-command",
                    "argument with spaces",
                    "",
                    R"(quote"and\slash)",
                },
            .cwd = typed::OptionalNullable<std::string>::withValue("./synthetic/../wire-cwd"),
            .disableOutputCap = false,
            .disableTimeout = true,
            .env = typed::OptionalNullable<std::map<std::string, std::optional<std::string>>>::withValue({
                {"EMPTY", std::string()},
                {"SET", std::string("synthetic-value")},
                {"UNSET", std::nullopt},
            }),
            .outputBytesCap = typed::OptionalNullable<std::uint64_t>::withValue(std::numeric_limits<std::uint64_t>::max()),
            .processId = typed::OptionalNullable<typed::CommandExecProcessId>::withValue(typed::CommandExecProcessId{"process-01"}),
            .sandboxPolicy = typed::OptionalNullable<typed::SandboxPolicy>::withValue(workspaceWrite),
            .size = typed::OptionalNullable<typed::CommandExecTerminalSize>::withValue({std::numeric_limits<std::uint16_t>::max(), 0}),
            .streamStdin = true,
            .streamStdoutStderr = false,
            .timeoutMs = typed::OptionalNullable<std::int64_t>::withValue(std::numeric_limits<std::int64_t>::min()),
            .tty = true,
        };
        const codex::Json expected{
            {"command", codex::Json::array({"synthetic-command", "argument with spaces", "", R"(quote"and\slash)"})},
            {"cwd", "./synthetic/../wire-cwd"},
            {"disableOutputCap", false},
            {"disableTimeout", true},
            {"env",
             {
                 {"EMPTY", ""},
                 {"SET", "synthetic-value"},
                 {"UNSET", nullptr},
             }},
            {"outputBytesCap", std::numeric_limits<std::uint64_t>::max()},
            {"processId", "process-01"},
            {"sandboxPolicy",
             {
                 {"excludeSlashTmp", false},
                 {"excludeTmpdirEnvVar", true},
                 {"networkAccess", false},
                 {"type", "workspaceWrite"},
                 {"writableRoots", codex::Json::array({"/synthetic/root-a", "/synthetic/root-b"})},
             }},
            {"size",
             {
                 {"cols", std::numeric_limits<std::uint16_t>::max()},
                 {"rows", 0},
             }},
            {"streamStdin", true},
            {"streamStdoutStderr", false},
            {"timeoutMs", std::numeric_limits<std::int64_t>::min()},
            {"tty", true},
        };
        const auto encoded = detail::encodeCommandExecParams(full, error);
        result.expectTrue(encoded == expected && error.empty() && !encoded->contains("permissionProfile"),
                          "command/exec preserves every argv element, nullable environment entry, "
                          "integer boundary, sandbox policy, and stable-only field");

        const typed::CommandExecParams explicitNulls{
            .command = {"synthetic-command"},
            .cwd = typed::OptionalNullable<std::string>::explicitNull(),
            .env = typed::OptionalNullable<std::map<std::string, std::optional<std::string>>>::explicitNull(),
            .outputBytesCap = typed::OptionalNullable<std::uint64_t>::explicitNull(),
            .processId = typed::OptionalNullable<typed::CommandExecProcessId>::explicitNull(),
            .sandboxPolicy = typed::OptionalNullable<typed::SandboxPolicy>::explicitNull(),
            .size = typed::OptionalNullable<typed::CommandExecTerminalSize>::explicitNull(),
            .timeoutMs = typed::OptionalNullable<std::int64_t>::explicitNull(),
        };
        const codex::Json expectedNulls{
            {"command", codex::Json::array({"synthetic-command"})},
            {"cwd", nullptr},
            {"env", nullptr},
            {"outputBytesCap", nullptr},
            {"processId", nullptr},
            {"sandboxPolicy", nullptr},
            {"size", nullptr},
            {"timeoutMs", nullptr},
        };
        result.expectTrue(detail::encodeCommandExecParams(explicitNulls, error) == expectedNulls && error.empty(),
                          "command/exec distinguishes explicit null from omission for all "
                          "nullable fields");

        typed::CommandExecParams inconsistent{.command = {"synthetic-command"}};
        inconsistent.cwd.value = "./must-not-encode";
        const auto rejected = detail::encodeCommandExecParams(inconsistent, error);
        result.expectTrue(!rejected && error.find("$.cwd") != std::string::npos && error.find("must-not-encode") == std::string::npos,
                          "command/exec rejects an inconsistent OptionalNullable state without "
                          "disclosing the path value");
    }

    void testFollowUpEncoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const typed::CommandExecProcessId processId{"process/with spaces"};

        const auto resize = detail::encodeCommandExecResizeParams(
            {processId, {std::numeric_limits<std::uint16_t>::max(), std::numeric_limits<std::uint16_t>::max()}}, error);
        result.expectTrue(resize ==
                                  codex::Json{
                                      {"processId", "process/with spaces"},
                                      {"size",
                                       {
                                           {"cols", std::numeric_limits<std::uint16_t>::max()},
                                           {"rows", std::numeric_limits<std::uint16_t>::max()},
                                       }},
                                  } &&
                              error.empty(),
                          "command/exec/resize preserves process identity and uint16 terminal bounds");

        const auto terminate = detail::encodeCommandExecTerminateParams({typed::CommandExecProcessId{""}}, error);
        result.expectTrue(terminate == codex::Json{{"processId", ""}} && error.empty(),
                          "command/exec/terminate preserves even an empty supplied process "
                          "identifier because the stable schema permits it");

        const auto minimumWrite = detail::encodeCommandExecWriteParams({.processId = processId}, error);
        result.expectTrue(minimumWrite == codex::Json{{"processId", "process/with spaces"}} && error.empty(),
                          "command/exec/write minimum request omits both optional fields");

        const typed::CommandExecWriteParams write{
            .processId = processId,
            .deltaBase64 = typed::OptionalNullable<std::string>::withValue("c3ludGhldGljLWlucHV0"),
            .closeStdin = false,
        };
        const auto fullWrite = detail::encodeCommandExecWriteParams(write, error);
        result.expectTrue(fullWrite ==
                                  codex::Json{
                                      {"closeStdin", false},
                                      {"deltaBase64", "c3ludGhldGljLWlucHV0"},
                                      {"processId", "process/with spaces"},
                                  } &&
                              error.empty(),
                          "command/exec/write preserves encoded stdin and present false closeStdin");

        typed::CommandExecWriteParams nullWrite{.processId = processId};
        nullWrite.deltaBase64 = typed::OptionalNullable<std::string>::explicitNull();
        nullWrite.closeStdin = true;
        result.expectTrue(detail::encodeCommandExecWriteParams(nullWrite, error) ==
                              codex::Json{
                                  {"closeStdin", true},
                                  {"deltaBase64", nullptr},
                                  {"processId", "process/with spaces"},
                              },
                          "command/exec/write preserves explicit-null stdin and explicit close");
    }

    void testResponseDecoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const codex::Json wire{
            {"exitCode", std::numeric_limits<std::int32_t>::min()},
            {"stderr", "synthetic-stderr"},
            {"stdout", "synthetic-stdout"},
            {"futureResultField", true},
        };
        const auto decoded = detail::decodeCommandExecResponse(wire, error);
        result.expectTrue(decoded && error.empty() && decoded->exitCode == std::numeric_limits<std::int32_t>::min() &&
                              decoded->stdoutData == "synthetic-stdout" && decoded->stderrData == "synthetic-stderr" &&
                              decoded->raw == wire,
                          "CommandExecResponse decodes all stable fields at the int32 lower bound "
                          "and retains future fields in raw");

        const codex::Json upperWire{
            {"exitCode", std::numeric_limits<std::int32_t>::max()},
            {"stderr", ""},
            {"stdout", ""},
        };
        const auto upper = detail::decodeCommandExecResponse(upperWire, error);
        result.expectTrue(upper && upper->exitCode == std::numeric_limits<std::int32_t>::max(),
                          "CommandExecResponse accepts the exact int32 upper bound");

        const auto missing = detail::decodeCommandExecResponse({{"exitCode", 0}, {"stderr", ""}}, error);
        result.expectTrue(!missing && error.find("$.stdout") != std::string::npos,
                          "CommandExecResponse rejects a missing required stdout field");

        const auto wrong = detail::decodeCommandExecResponse({{"exitCode", 0}, {"stderr", codex::Json::array()}, {"stdout", ""}}, error);
        result.expectTrue(!wrong && error.find("$.stderr") != std::string::npos,
                          "CommandExecResponse rejects a wrong scalar type at its path");

        const auto above = detail::decodeCommandExecResponse(
            {
                {"exitCode", static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) + 1U},
                {"stderr", ""},
                {"stdout", ""},
            },
            error);
        result.expectTrue(!above && error.find("$.exitCode") != std::string::npos, "CommandExecResponse rejects an integer above int32");

        const auto below = detail::decodeCommandExecResponse(
            {
                {"exitCode", static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()) - 1},
                {"stderr", ""},
                {"stdout", ""},
            },
            error);
        result.expectTrue(!below && error.find("$.exitCode") != std::string::npos, "CommandExecResponse rejects an integer below int32");
    }

    void testOutputNotificationDecoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const codex::Notification stdout = outputNotification({
            {"capReached", false},
            {"deltaBase64", "c3ludGhldGljLW91dHB1dA=="},
            {"processId", "process-output"},
            {"stream", "stdout"},
            {"futureParam", true},
        });
        const auto decoded = detail::decodeCommandExecOutputDeltaNotification(stdout, error);
        result.expectTrue(decoded && error.empty() && !decoded->capReached && decoded->deltaBase64 == "c3ludGhldGljLW91dHB1dA==" &&
                              decoded->processId.value == "process-output" &&
                              decoded->stream == typed::CommandExecOutputStream::stdoutStream() && decoded->raw == stdout.raw &&
                              decoded->diagnostics.empty(),
                          "command output notification decodes stdout identity and retains its exact "
                          "raw envelope");

        const codex::Notification future = outputNotification({
            {"capReached", true},
            {"deltaBase64", ""},
            {"processId", "process-output"},
            {"stream", "future-stream"},
        });
        const auto futureDecoded = detail::decodeCommandExecOutputDeltaNotification(future, error);
        result.expectTrue(futureDecoded && futureDecoded->capReached && futureDecoded->stream.value == "future-stream" &&
                              !futureDecoded->stream.isKnown() &&
                              hasDiagnostic(futureDecoded->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "CommandExecOutputStream",
                                            "$.params.stream"),
                          "command output notification preserves and diagnoses a future stream value");

        const auto missing = detail::decodeCommandExecOutputDeltaNotification(outputNotification({
                                                                                  {"capReached", false},
                                                                                  {"processId", "process-output"},
                                                                                  {"stream", "stderr"},
                                                                              }),
                                                                              error);
        result.expectTrue(!missing && error.find("$.deltaBase64") != std::string::npos,
                          "command output notification rejects a missing required field");

        const auto wrong = detail::decodeCommandExecOutputDeltaNotification(outputNotification({
                                                                                {"capReached", "no"},
                                                                                {"deltaBase64", ""},
                                                                                {"processId", "process-output"},
                                                                                {"stream", "stderr"},
                                                                            }),
                                                                            error);
        result.expectTrue(!wrong && error.find("$.capReached") != std::string::npos,
                          "command output notification rejects a wrong scalar type");

        const typed::Event typedEvent = detail::decodeEvent(stdout);
        result.expectTrue(std::holds_alternative<typed::CommandExecOutputDeltaNotification>(typedEvent),
                          "the existing event decoder dispatches command output through the canonical "
                          "Event variant");

        const typed::Event malformedEvent = detail::decodeEvent(outputNotification({
            {"capReached", false},
            {"deltaBase64", 7},
            {"processId", "process-output"},
            {"stream", "stdout"},
        }));
        const auto* unknown = std::get_if<typed::UnknownEvent>(&malformedEvent);
        result.expectTrue(unknown && unknown->method == "command/exec/outputDelta" && unknown->diagnostic &&
                              unknown->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              unknown->diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning &&
                              unknown->raw.at("params").at("deltaBase64") == 7,
                          "a malformed known command output remains observable as a protocol warning");
    }

    void testOperationAssociations(tests::support::TestResult& result) {
        const codex::Json response{
            {"exitCode", 0},
            {"stderr", ""},
            {"stdout", "synthetic-output"},
        };
        const auto exec = detail::decodeClientOperationResult(detail::ClientRequestTarget::CommandExec, response);
        const auto resize = detail::decodeClientOperationResult(detail::ClientRequestTarget::CommandExecResize, codex::Json::object());
        const auto terminate =
            detail::decodeClientOperationResult(detail::ClientRequestTarget::CommandExecTerminate, codex::Json::object());
        const auto write = detail::decodeClientOperationResult(detail::ClientRequestTarget::CommandExecWrite, codex::Json::object());
        result.expectTrue(exec && std::holds_alternative<typed::CommandExecResponse>(*exec.value) && resize &&
                              std::holds_alternative<typed::Unit>(*resize.value) && terminate &&
                              std::holds_alternative<typed::Unit>(*terminate.value) && write &&
                              std::holds_alternative<typed::Unit>(*write.value),
                          "all four command targets select their authoritative Concrete/Unit result "
                          "decoder");

        const auto malformedUnit =
            detail::decodeClientOperationResult(detail::ClientRequestTarget::CommandExecWrite, {{"unexpected", true}});
        result.expectTrue(!malformedUnit && malformedUnit.diagnostic.code == detail::ClientOperationDecodeCode::MalformedKnownPayload &&
                              malformedUnit.diagnostic.message == "Unit successful result must be the exact empty object",
                          "command follow-up operations preserve the exact-empty-object Unit invariant");

        const auto wrongAssociation = detail::decodeClientOperationResult(detail::ClientRequestTarget::CommandExec, codex::Json::object());
        result.expectTrue(!wrongAssociation && wrongAssociation.diagnostic.code == detail::ClientOperationDecodeCode::MalformedKnownPayload,
                          "a wrong command result shape fails through the target-specific decoder");
    }
} // namespace

int main() {
    tests::support::TestResult result;
    testOpenStreamValues(result);
    testExecEncoding(result);
    testFollowUpEncoding(result);
    testResponseDecoding(result);
    testOutputNotificationDecoding(result);
    testOperationAssociations(result);
    return result.processResult();
}
