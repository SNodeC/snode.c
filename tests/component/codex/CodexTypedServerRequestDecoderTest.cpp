/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ServerRequestDecoder.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <variant>

namespace {
    using ai::openai::codex::Json;
    using ai::openai::codex::ServerRequest;
    using ai::openai::codex::ServerRequestId;
    using ai::openai::codex::detail::decodeServerRequest;
    using ai::openai::codex::typed::AuthenticationRequest;
    using ai::openai::codex::typed::CommandApprovalRequest;
    using ai::openai::codex::typed::FileChangeApprovalRequest;
    using ai::openai::codex::typed::TypedServerRequest;
    using ai::openai::codex::typed::UnknownServerRequest;
    using ai::openai::codex::typed::UserInputRequest;

    Json encodeRequestId(const ServerRequestId& requestId) {
        return std::visit(
            [](const auto& value) -> Json {
                return value;
            },
            requestId.value());
    }

    ServerRequest makeRequest(ServerRequestId id, std::string method, Json params, Json envelopeExtension = Json::object()) {
        Json raw = {
            {"id", encodeRequestId(id)},
            {"method", method},
            {"params", params},
            {"futureEnvelope", std::move(envelopeExtension)},
        };
        return {std::move(id), std::move(method), std::move(params), std::move(raw)};
    }

    bool hasIntegerId(const ServerRequestId& id, std::int64_t expected) {
        return std::holds_alternative<std::int64_t>(id.value()) && std::get<std::int64_t>(id.value()) == expected;
    }

    bool hasStringId(const ServerRequestId& id, const std::string& expected) {
        return std::holds_alternative<std::string>(id.value()) && std::get<std::string>(id.value()) == expected;
    }

    void testCommandApproval(tests::support::TestResult& testResult) {
        const Json params = {
            {"threadId", "thread-command"},
            {"turnId", "turn-command"},
            {"itemId", "item-command"},
            {"startedAtMs", 1'234},
            {"command", "git status --short"},
            {"cwd", "/tmp/project"},
            {"reason", "inspect the working tree"},
            {"approvalId", "approval-callback-1"},
            {"futureParams", Json::array({true, 7, "kept"})},
        };
        const ServerRequest rawRequest =
            makeRequest(ServerRequestId{41}, "item/commandExecution/requestApproval", params, Json{{"source", "future"}});
        const TypedServerRequest decoded = decodeServerRequest(rawRequest);
        const CommandApprovalRequest* approval = std::get_if<CommandApprovalRequest>(&decoded);

        testResult.expectTrue(approval != nullptr, "command approval method is classified");
        testResult.expectTrue(approval && hasIntegerId(approval->requestId, 41), "command approval preserves an integer request ID");
        testResult.expectTrue(approval && approval->threadId.value == "thread-command" && approval->turnId.value == "turn-command" &&
                                  approval->itemId.value == "item-command",
                              "command approval preserves required typed IDs");
        testResult.expectTrue(approval && approval->startedAtMs == 1'234, "command approval preserves startedAtMs");
        testResult.expectTrue(approval && approval->command == "git status --short" && approval->cwd == "/tmp/project" &&
                                  approval->reason == "inspect the working tree",
                              "command approval decodes optional display fields");
        testResult.expectTrue(approval && approval->details == params,
                              "command approval details preserve every parameter including future fields");
        testResult.expectTrue(approval && approval->raw == rawRequest.raw,
                              "command approval preserves the exact complete raw request envelope");
    }

    void testFileChangeApproval(tests::support::TestResult& testResult) {
        const Json params = {
            {"threadId", "thread-file"},
            {"turnId", "turn-file"},
            {"itemId", "item-file"},
            {"startedAtMs", -1},
            {"reason", nullptr},
            {"grantRoot", "/tmp/project"},
            {"futureParams", {"preserved", 8}},
        };
        const ServerRequest rawRequest = makeRequest(
            ServerRequestId{std::string("file-approval-id")}, "item/fileChange/requestApproval", params, Json::array({1, 2, 3}));
        const TypedServerRequest decoded = decodeServerRequest(rawRequest);
        const FileChangeApprovalRequest* approval = std::get_if<FileChangeApprovalRequest>(&decoded);

        testResult.expectTrue(approval != nullptr, "file-change approval method is classified");
        testResult.expectTrue(approval && hasStringId(approval->requestId, "file-approval-id"),
                              "file-change approval preserves a string request ID");
        testResult.expectTrue(approval && approval->threadId.value == "thread-file" && approval->turnId.value == "turn-file" &&
                                  approval->itemId.value == "item-file" && approval->startedAtMs == -1,
                              "file-change approval preserves required IDs and signed timestamp");
        testResult.expectTrue(approval && !approval->reason && approval->grantRoot == "/tmp/project",
                              "file-change approval accepts null and string optional fields");
        testResult.expectTrue(approval && approval->raw == rawRequest.raw,
                              "file-change approval preserves the exact complete raw request envelope and future fields");
    }

    Json userInputParams() {
        return {
            {"threadId", "thread-input"},
            {"turnId", "turn-input"},
            {"itemId", "item-input"},
            {"questions",
             Json::array({
                 Json{{"id", "choice"},
                      {"header", "Choice"},
                      {"question", "Choose a mode"},
                      {"isOther", true},
                      {"isSecret", true},
                      {"options",
                       Json::array({Json{{"label", "Safe"}, {"description", "Use the safe mode"}, {"futureOption", 1}},
                                    Json{{"label", "Fast"}, {"description", "Use the fast mode"}}})},
                      {"futureQuestion", "kept"}},
                 Json{{"id", "free"}, {"header", "Details"}, {"question", "Add details"}},
                 Json{{"id", "nullable"}, {"header", "Optional"}, {"question", "Anything else?"}, {"options", nullptr}},
             })},
            {"autoResolutionMs", std::numeric_limits<std::uint64_t>::max()},
            {"futureParams", {"still", "typed"}},
        };
    }

    void testUserInput(tests::support::TestResult& testResult) {
        const Json params = userInputParams();
        const ServerRequest rawRequest = makeRequest(ServerRequestId{42}, "item/tool/requestUserInput", params, "future-input");
        const TypedServerRequest decoded = decodeServerRequest(rawRequest);
        const UserInputRequest* input = std::get_if<UserInputRequest>(&decoded);

        testResult.expectTrue(input != nullptr, "user-input method is classified");
        testResult.expectTrue(input && hasIntegerId(input->requestId, 42) && input->threadId.value == "thread-input" &&
                                  input->turnId.value == "turn-input" && input->itemId.value == "item-input",
                              "user-input request preserves request and workflow IDs");
        testResult.expectTrue(input && input->autoResolutionMs == std::numeric_limits<std::uint64_t>::max(),
                              "user-input request preserves the complete uint64 auto-resolution range");
        testResult.expectTrue(input && input->questions.size() == 3, "user-input request decodes every question");
        testResult.expectTrue(input && !input->questions.empty() && input->questions[0].id == "choice" &&
                                  input->questions[0].header == "Choice" && input->questions[0].prompt == "Choose a mode" &&
                                  input->questions[0].allowsFreeText && input->questions[0].secret,
                              "user-input question decodes required text and optional flags");
        testResult.expectTrue(input && !input->questions.empty() && input->questions[0].options.size() == 2 &&
                                  input->questions[0].options[0].label == "Safe" &&
                                  input->questions[0].options[0].description == "Use the safe mode" &&
                                  input->questions[0].options[0].raw == params["questions"][0]["options"][0],
                              "user-input options validate required fields and preserve exact raw values");
        testResult.expectTrue(input && input->questions.size() >= 3 && input->questions[1].options.empty() &&
                                  !input->questions[1].allowsFreeText && !input->questions[1].secret && input->questions[2].options.empty(),
                              "missing flags use schema defaults and missing or null options decode as empty");
        testResult.expectTrue(input && !input->questions.empty() && input->questions[0].raw == params["questions"][0],
                              "user-input question preserves future fields in raw");
        testResult.expectTrue(input && input->raw == rawRequest.raw,
                              "user-input request preserves the exact complete raw request envelope");
    }

    void testAuthentication(tests::support::TestResult& testResult) {
        const Json params = {
            {"reason", "unauthorized"},
            {"previousAccountId", "workspace-account"},
            {"futureAuthHint", Json{{"retry", true}}},
        };
        const ServerRequest rawRequest = makeRequest(
            ServerRequestId{std::string("auth-refresh-id")}, "account/chatgptAuthTokens/refresh", params, Json{{"authEnvelope", true}});
        const TypedServerRequest decoded = decodeServerRequest(rawRequest);
        const AuthenticationRequest* authentication = std::get_if<AuthenticationRequest>(&decoded);

        testResult.expectTrue(authentication != nullptr, "ChatGPT authentication-token refresh method is classified");
        testResult.expectTrue(authentication && hasStringId(authentication->requestId, "auth-refresh-id"),
                              "authentication request preserves a string request ID");
        testResult.expectTrue(authentication && authentication->reason == "unauthorized" &&
                                  authentication->previousAccountId == "workspace-account",
                              "authentication request decodes required reason and optional account hint");
        testResult.expectTrue(authentication && authentication->raw == rawRequest.raw,
                              "authentication request preserves the exact complete raw request envelope and future fields");
    }

    void testUnknownAndMalformedRequests(tests::support::TestResult& testResult) {
        const Json unknownParams = Json::array({1, "two", false});
        const ServerRequest rawUnknown =
            makeRequest(ServerRequestId{std::string("future-id")}, "future/server/request", unknownParams, Json{{"version", 2}});
        const TypedServerRequest decodedUnknown = decodeServerRequest(rawUnknown);
        const UnknownServerRequest* unknown = std::get_if<UnknownServerRequest>(&decodedUnknown);

        testResult.expectTrue(unknown != nullptr, "unknown request method decodes as UnknownServerRequest");
        testResult.expectTrue(unknown && hasStringId(unknown->requestId, "future-id") && unknown->method == "future/server/request",
                              "unknown request remains answerable with its exact request ID and method");
        testResult.expectTrue(unknown && unknown->params == unknownParams && unknown->raw == rawUnknown.raw && !unknown->decodingError,
                              "unknown request preserves exact params and complete raw envelope without a decoding error");

        Json missingIdParams = {
            {"threadId", "thread-malformed"},
            {"turnId", "turn-malformed"},
            {"startedAtMs", 9},
        };
        const ServerRequest rawMissingId =
            makeRequest(ServerRequestId{43}, "item/commandExecution/requestApproval", missingIdParams, "malformed");
        const TypedServerRequest decodedMissingId = decodeServerRequest(rawMissingId);
        const UnknownServerRequest* missingId = std::get_if<UnknownServerRequest>(&decodedMissingId);
        testResult.expectTrue(missingId && missingId->decodingError && !missingId->decodingError->empty(),
                              "known request missing a required typed ID falls back to unknown with a decode reason");
        testResult.expectTrue(missingId && hasIntegerId(missingId->requestId, 43) && missingId->params == missingIdParams &&
                                  missingId->raw == rawMissingId.raw,
                              "malformed known request remains answerable and preserves exact params and raw envelope");

        Json malformedOptions = userInputParams();
        malformedOptions["questions"][0]["options"][0].erase("description");
        const ServerRequest rawMalformedOptions =
            makeRequest(ServerRequestId{44}, "item/tool/requestUserInput", malformedOptions, "bad-options");
        const TypedServerRequest decodedMalformedOptions = decodeServerRequest(rawMalformedOptions);
        const UnknownServerRequest* badOptions = std::get_if<UnknownServerRequest>(&decodedMalformedOptions);
        testResult.expectTrue(badOptions && badOptions->decodingError && !badOptions->decodingError->empty(),
                              "user-input option missing a required field falls back to unknown with a decode reason");
        testResult.expectTrue(badOptions && badOptions->raw == rawMalformedOptions.raw,
                              "malformed user-input option retains the exact complete raw request");

        Json negativeTimeout = userInputParams();
        negativeTimeout["autoResolutionMs"] = -1;
        const ServerRequest rawNegativeTimeout =
            makeRequest(ServerRequestId{45}, "item/tool/requestUserInput", negativeTimeout, "bad-timeout");
        const TypedServerRequest decodedNegativeTimeout = decodeServerRequest(rawNegativeTimeout);
        const UnknownServerRequest* badTimeout = std::get_if<UnknownServerRequest>(&decodedNegativeTimeout);
        testResult.expectTrue(badTimeout && badTimeout->decodingError && !badTimeout->decodingError->empty(),
                              "negative autoResolutionMs falls back to unknown with a decode reason");
        testResult.expectTrue(badTimeout && badTimeout->params == negativeTimeout && badTimeout->raw == rawNegativeTimeout.raw,
                              "invalid uint64 input retains exact params and complete raw request");
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    testCommandApproval(testResult);
    testFileChangeApproval(testResult);
    testUserInput(testResult);
    testAuthentication(testResult);
    testUnknownAndMalformedRequests(testResult);

    return testResult.processResult();
}
