/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/ProtocolCodec.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
    using ai::openai::codex::ClientInfo;
    using ai::openai::codex::InitializeResult;
    using ai::openai::codex::Json;
    using ai::openai::codex::ProtocolError;
    using ai::openai::codex::detail::ProtocolCodec;
    using ai::openai::codex::detail::ProtocolId;
    using ai::openai::codex::detail::ProtocolMessage;

    bool hasIntegerId(const std::optional<ProtocolId>& id, std::int64_t expected) {
        return id && std::holds_alternative<std::int64_t>(*id) && std::get<std::int64_t>(*id) == expected;
    }

    bool hasStringId(const std::optional<ProtocolId>& id, const std::string& expected) {
        return id && std::holds_alternative<std::string>(*id) && std::get<std::string>(*id) == expected;
    }

    std::optional<ProtocolMessage> decode(const Json& message, std::string& errorMessage) {
        return ProtocolCodec::decode(message.dump(), errorMessage);
    }

    void expectWireRejected(tests::support::TestResult& testResult, const std::string& wireMessage, const std::string& description) {
        std::string errorMessage;
        const std::optional<ProtocolMessage> decoded = ProtocolCodec::decode(wireMessage, errorMessage);
        testResult.expectTrue(!decoded, description + " is rejected");
        testResult.expectTrue(!errorMessage.empty(), description + " reports a decode reason");
    }

    void expectRejected(tests::support::TestResult& testResult, const Json& message, const std::string& description) {
        expectWireRejected(testResult, message.dump(), description);
    }

    void testRequestsAndNotifications(tests::support::TestResult& testResult) {
        const Json integerRequest = {
            {"method", "thread/list"},
            {"id", 17},
            {"params", {{"cursor", nullptr}}},
            {"futureField", Json::array({1, "two"})},
        };
        std::string errorMessage;
        const std::optional<ProtocolMessage> decodedIntegerRequest = decode(integerRequest, errorMessage);
        testResult.expectTrue(decodedIntegerRequest && decodedIntegerRequest->kind == ProtocolMessage::Kind::Request,
                              "integer-ID request is decoded as a request");
        testResult.expectTrue(decodedIntegerRequest && hasIntegerId(decodedIntegerRequest->id, 17),
                              "integer request ID is preserved as signed 64-bit");
        testResult.expectTrue(decodedIntegerRequest && decodedIntegerRequest->method == "thread/list",
                              "integer-ID request method is preserved");
        testResult.expectTrue(decodedIntegerRequest && decodedIntegerRequest->params == integerRequest["params"],
                              "request object parameters are preserved");
        testResult.expectTrue(decodedIntegerRequest && decodedIntegerRequest->raw == integerRequest,
                              "request raw envelope and unknown fields are preserved");

        const Json stringRequest = {
            {"method", "item/commandExecution/requestApproval"},
            {"id", "approval-123"},
            {"params", Json::array({"first", 2, false, nullptr})},
        };
        errorMessage = "stale";
        const std::optional<ProtocolMessage> decodedStringRequest = decode(stringRequest, errorMessage);
        testResult.expectTrue(decodedStringRequest && decodedStringRequest->kind == ProtocolMessage::Kind::Request,
                              "string-ID request is decoded as a request");
        testResult.expectTrue(decodedStringRequest && hasStringId(decodedStringRequest->id, "approval-123"),
                              "string request ID is preserved exactly");
        testResult.expectTrue(decodedStringRequest && decodedStringRequest->params == stringRequest["params"],
                              "request array parameters are accepted");
        testResult.expectTrue(decodedStringRequest && decodedStringRequest->raw == stringRequest,
                              "string-ID request retains its complete raw envelope");
        testResult.expectTrue(errorMessage.empty(), "successful request decoding clears a previous error");

        const Json notification = {
            {"method", "future/notification"},
            {"params", true},
            {"jsonrpc", "2.0"},
            {"extension", {{"enabled", true}}},
        };
        const std::optional<ProtocolMessage> decodedNotification = decode(notification, errorMessage);
        testResult.expectTrue(decodedNotification && decodedNotification->kind == ProtocolMessage::Kind::Notification,
                              "notification without ID is decoded as a notification");
        testResult.expectTrue(decodedNotification && !decodedNotification->id, "notification does not synthesize a request ID");
        testResult.expectTrue(decodedNotification && decodedNotification->method == "future/notification",
                              "unknown notification method is accepted");
        testResult.expectTrue(decodedNotification && decodedNotification->params == true, "notification Boolean parameters are accepted");
        testResult.expectTrue(decodedNotification && decodedNotification->raw == notification,
                              "notification raw envelope, JSON-RPC marker, and extensions are preserved");

        const Json notificationWithoutParams = {{"method", "future/withoutParams"}};
        const std::optional<ProtocolMessage> decodedWithoutParams = decode(notificationWithoutParams, errorMessage);
        testResult.expectTrue(decodedWithoutParams && decodedWithoutParams->kind == ProtocolMessage::Kind::Notification &&
                                  decodedWithoutParams->params.is_null(),
                              "missing notification params decode as null without rejecting an unknown method");

        const Json minimumIntegerRequest = {
            {"method", "future/minimumId"},
            {"id", std::numeric_limits<std::int64_t>::min()},
        };
        const std::optional<ProtocolMessage> decodedMinimumIntegerRequest = decode(minimumIntegerRequest, errorMessage);
        testResult.expectTrue(decodedMinimumIntegerRequest &&
                                  hasIntegerId(decodedMinimumIntegerRequest->id, std::numeric_limits<std::int64_t>::min()),
                              "minimum signed 64-bit request ID is accepted");

        const Json maximumIntegerRequest = {
            {"method", "future/maximumId"},
            {"id", std::numeric_limits<std::int64_t>::max()},
        };
        const std::optional<ProtocolMessage> decodedMaximumIntegerRequest = decode(maximumIntegerRequest, errorMessage);
        testResult.expectTrue(decodedMaximumIntegerRequest &&
                                  hasIntegerId(decodedMaximumIntegerRequest->id, std::numeric_limits<std::int64_t>::max()),
                              "maximum signed 64-bit request ID is accepted");
    }

    void testResultResponses(tests::support::TestResult& testResult) {
        const std::vector<std::pair<std::string, Json>> resultCases = {
            {"object", Json{{"data", Json::array({1, 2})}}},
            {"array", Json::array({Json{{"id", "a"}}, Json{{"id", "b"}}})},
            {"string scalar", Json("complete")},
            {"number scalar", Json(42.5)},
            {"Boolean", Json(false)},
            {"null", Json(nullptr)},
        };

        std::int64_t id = 100;
        for (const auto& [description, result] : resultCases) {
            const Json envelope = {
                {"id", id},
                {"result", result},
                {"futureResponseField", {{"case", description}}},
            };
            std::string errorMessage;
            const std::optional<ProtocolMessage> decoded = decode(envelope, errorMessage);
            testResult.expectTrue(decoded && decoded->kind == ProtocolMessage::Kind::Response,
                                  description + " result is decoded as a response");
            testResult.expectTrue(decoded && hasIntegerId(decoded->id, id), description + " response retains its integer ID");
            testResult.expectTrue(decoded && !decoded->error, description + " result is not misclassified as a remote error");
            testResult.expectTrue(decoded && decoded->result == result, description + " result JSON is preserved exactly");
            testResult.expectTrue(decoded && decoded->raw == envelope,
                                  description + " response preserves the full raw envelope and unknown fields");
            ++id;
        }

        const Json stringIdResponse = {
            {"id", "server-originated-id"},
            {"result", Json::array()},
        };
        std::string errorMessage;
        const std::optional<ProtocolMessage> decodedStringIdResponse = decode(stringIdResponse, errorMessage);
        testResult.expectTrue(decodedStringIdResponse && decodedStringIdResponse->kind == ProtocolMessage::Kind::Response,
                              "string-ID response is structurally valid");
        testResult.expectTrue(decodedStringIdResponse && hasStringId(decodedStringIdResponse->id, "server-originated-id"),
                              "string response ID representation is preserved exactly");
    }

    void testRemoteErrors(tests::support::TestResult& testResult) {
        const Json errorWithData = {
            {"id", 201},
            {"error", {{"code", -32000}, {"message", "failure"}, {"data", {{"reason", "example"}}}, {"future", 1}}},
            {"responseExtension", true},
        };
        std::string errorMessage;
        const std::optional<ProtocolMessage> decodedWithData = decode(errorWithData, errorMessage);
        testResult.expectTrue(decodedWithData && decodedWithData->kind == ProtocolMessage::Kind::Response && decodedWithData->error,
                              "remote error with data is decoded as an error response");
        testResult.expectTrue(decodedWithData && decodedWithData->error && decodedWithData->error->code == -32000 &&
                                  decodedWithData->error->message == "failure",
                              "remote error code and message are preserved");
        testResult.expectTrue(decodedWithData && decodedWithData->error && decodedWithData->error->data &&
                                  *decodedWithData->error->data == Json{{"reason", "example"}},
                              "remote error object data is preserved");
        testResult.expectTrue(decodedWithData && decodedWithData->raw == errorWithData,
                              "remote error retains unknown nested and envelope fields in raw");

        const Json errorWithNullData = {
            {"id", 202},
            {"error", {{"code", 7}, {"message", "null data"}, {"data", nullptr}}},
        };
        const std::optional<ProtocolMessage> decodedWithNullData = decode(errorWithNullData, errorMessage);
        testResult.expectTrue(decodedWithNullData && decodedWithNullData->error && decodedWithNullData->error->data.has_value() &&
                                  decodedWithNullData->error->data->is_null(),
                              "explicit null remote error data remains present and null");

        const Json errorWithoutData = {
            {"id", 203},
            {"error", {{"code", -1}, {"message", "no data"}}},
        };
        const std::optional<ProtocolMessage> decodedWithoutData = decode(errorWithoutData, errorMessage);
        testResult.expectTrue(decodedWithoutData && decodedWithoutData->error && !decodedWithoutData->error->data,
                              "absent remote error data remains absent");

        const Json errorWithScalarData = {
            {"id", "error-id"},
            {"error", {{"code", 0}, {"message", "scalar data"}, {"data", false}}},
        };
        const std::optional<ProtocolMessage> decodedWithScalarData = decode(errorWithScalarData, errorMessage);
        testResult.expectTrue(decodedWithScalarData && hasStringId(decodedWithScalarData->id, "error-id") && decodedWithScalarData->error &&
                                  decodedWithScalarData->error->data && *decodedWithScalarData->error->data == false,
                              "remote error accepts arbitrary scalar data with a string response ID");
    }

    void testMalformedAndAmbiguousMessages(tests::support::TestResult& testResult) {
        expectWireRejected(testResult, "{not-json}", "malformed JSON");
        expectRejected(testResult, Json::array({1, 2}), "non-object top-level message");
        expectRejected(testResult, Json{{"result", Json::object()}}, "response missing an ID");
        expectRejected(testResult,
                       Json{{"id", 1}, {"result", Json::object()}, {"error", {{"code", -1}, {"message", "both"}}}},
                       "response containing both result and error");
        expectRejected(testResult, Json{{"id", 1}}, "response containing neither result nor error");
        expectRejected(testResult, Json{{"method", 17}, {"params", Json::object()}}, "non-string method");
        expectRejected(testResult, Json{{"method", "future/request"}, {"id", false}}, "Boolean request ID");
        expectRejected(testResult, Json{{"id", Json::array({1})}, {"result", nullptr}}, "array response ID");
        expectRejected(testResult, Json{{"id", 1.5}, {"result", nullptr}}, "floating-point response ID");
        expectWireRejected(testResult,
                           R"({"method":"future/request","id":18446744073709551615,"params":{}})",
                           "unsigned request ID outside signed 64-bit range");
        expectRejected(testResult,
                       Json{{"method", "future/ambiguous"}, {"id", 5}, {"params", Json::object()}, {"result", nullptr}},
                       "method envelope also containing a result");
        expectRejected(testResult,
                       Json{{"method", "future/ambiguous"}, {"error", {{"code", -1}, {"message", "response-shaped notification"}}}},
                       "method envelope also containing an error");
        expectRejected(testResult, Json::object(), "empty protocol envelope");
        expectRejected(testResult, Json{{"id", 2}, {"error", "not an object"}}, "remote error whose payload is not an object");
        expectRejected(testResult,
                       Json{{"id", 3}, {"error", {{"code", "not an integer"}, {"message", "failure"}}}},
                       "remote error with a non-integer code");
        expectRejected(
            testResult, Json{{"id", 4}, {"error", {{"code", -1}, {"message", Json::object()}}}}, "remote error with a non-string message");
        std::string maximumErrorMessage;
        const auto decodedMaximumErrorCode = decode(
            Json{{"id", 5}, {"error", {{"code", std::numeric_limits<std::int64_t>::max()}, {"message", "maximum"}}}}, maximumErrorMessage);
        testResult.expectTrue(decodedMaximumErrorCode && decodedMaximumErrorCode->error &&
                                  decodedMaximumErrorCode->error->code == std::numeric_limits<std::int64_t>::max(),
                              "maximum signed 64-bit remote error code is preserved");
        testResult.expectTrue(maximumErrorMessage.empty(), "maximum signed 64-bit remote error code has no decode error");

        std::string minimumErrorMessage;
        const auto decodedMinimumErrorCode = decode(
            Json{{"id", 6}, {"error", {{"code", std::numeric_limits<std::int64_t>::min()}, {"message", "minimum"}}}}, minimumErrorMessage);
        testResult.expectTrue(decodedMinimumErrorCode && decodedMinimumErrorCode->error &&
                                  decodedMinimumErrorCode->error->code == std::numeric_limits<std::int64_t>::min(),
                              "minimum signed 64-bit remote error code is preserved");
        testResult.expectTrue(minimumErrorMessage.empty(), "minimum signed 64-bit remote error code has no decode error");
    }

    void testGenericEncoders(tests::support::TestResult& testResult) {
        std::string errorMessage = "stale";
        const Json requestParams = Json::array({Json{{"type", "text"}}, nullptr});
        const std::optional<std::string> encodedRequest = ProtocolCodec::encodeRequest(301, "turn/start", requestParams, errorMessage);
        const Json expectedRequest = {{"method", "turn/start"}, {"id", 301}, {"params", requestParams}};
        testResult.expectTrue(encodedRequest && Json::parse(*encodedRequest) == expectedRequest,
                              "generic request encoder preserves method, integer ID, and arbitrary params");
        testResult.expectTrue(encodedRequest && encodedRequest->find('\n') == std::string::npos &&
                                  encodedRequest->find('\r') == std::string::npos,
                              "generic request encoder emits an unframed JSON document");
        testResult.expectTrue(errorMessage.empty(), "successful request encoding clears a previous error");

        const Json notificationParams = "delta";
        const std::optional<std::string> encodedNotification =
            ProtocolCodec::encodeNotification("future/notification", notificationParams, errorMessage);
        const Json expectedNotification = {{"method", "future/notification"}, {"params", notificationParams}};
        testResult.expectTrue(encodedNotification && Json::parse(*encodedNotification) == expectedNotification,
                              "generic notification encoder preserves an unknown method and scalar params");
        testResult.expectTrue(encodedNotification && encodedNotification->find('\n') == std::string::npos &&
                                  encodedNotification->find('\r') == std::string::npos,
                              "generic notification encoder emits an unframed JSON document");

        const ProtocolId integerResponseId = std::int64_t{302};
        const Json successResult = Json{{"accepted", true}};
        const std::optional<std::string> encodedIntegerSuccess =
            ProtocolCodec::encodeSuccessResponse(integerResponseId, successResult, errorMessage);
        const Json expectedIntegerSuccess = {{"id", 302}, {"result", successResult}};
        testResult.expectTrue(encodedIntegerSuccess && Json::parse(*encodedIntegerSuccess) == expectedIntegerSuccess,
                              "generic success encoder preserves an integer server-request ID and result");

        const ProtocolId stringResponseId = std::string("approval-302");
        const Json arrayResult = Json::array({"accepted", 1, nullptr});
        const std::optional<std::string> encodedStringSuccess =
            ProtocolCodec::encodeSuccessResponse(stringResponseId, arrayResult, errorMessage);
        const Json expectedStringSuccess = {{"id", "approval-302"}, {"result", arrayResult}};
        testResult.expectTrue(encodedStringSuccess && Json::parse(*encodedStringSuccess) == expectedStringSuccess,
                              "generic success encoder preserves a string server-request ID exactly");

        const ProtocolError rejection = {
            .code = -32000,
            .message = "Request rejected",
            .data = Json{{"reason", "policy"}},
        };
        const std::optional<std::string> encodedRejection = ProtocolCodec::encodeErrorResponse(stringResponseId, rejection, errorMessage);
        const Json expectedRejection = {
            {"id", "approval-302"},
            {"error", {{"code", -32000}, {"message", "Request rejected"}, {"data", {{"reason", "policy"}}}}},
        };
        testResult.expectTrue(encodedRejection && Json::parse(*encodedRejection) == expectedRejection,
                              "generic rejection encoder preserves ID, error fields, and data");
        testResult.expectTrue(encodedRejection && encodedRejection->find('\n') == std::string::npos &&
                                  encodedRejection->find('\r') == std::string::npos,
                              "generic error encoder emits an unframed JSON document");

        const ProtocolError rejectionWithoutData = {
            .code = -1,
            .message = "without data",
            .data = std::nullopt,
        };
        const std::optional<std::string> encodedWithoutData =
            ProtocolCodec::encodeErrorResponse(integerResponseId, rejectionWithoutData, errorMessage);
        testResult.expectTrue(encodedWithoutData && !Json::parse(*encodedWithoutData)["error"].contains("data"),
                              "generic rejection encoder omits absent error data");

        const ProtocolError rejectionWithNullData = {
            .code = -2,
            .message = "null data",
            .data = Json(nullptr),
        };
        const std::optional<std::string> encodedWithNullData =
            ProtocolCodec::encodeErrorResponse(integerResponseId, rejectionWithNullData, errorMessage);
        testResult.expectTrue(encodedWithNullData && Json::parse(*encodedWithNullData)["error"].contains("data") &&
                                  Json::parse(*encodedWithNullData)["error"]["data"].is_null(),
                              "generic rejection encoder preserves explicitly null error data");

        const Json invalidUtf8 = std::string(1, static_cast<char>(0xff));
        errorMessage.clear();
        const std::optional<std::string> invalidEncoding =
            ProtocolCodec::encodeRequest(303, "future/invalidUtf8", invalidUtf8, errorMessage);
        testResult.expectTrue(!invalidEncoding && !errorMessage.empty(), "encoder reports invalid JSON string encoding safely");

        const Json discarded = Json::parse("{", nullptr, false);
        errorMessage.clear();
        const std::optional<std::string> discardedEncoding =
            ProtocolCodec::encodeNotification("future/discarded", Json{{"nested", discarded}}, errorMessage);
        testResult.expectTrue(!discardedEncoding && !errorMessage.empty(),
                              "encoder rejects discarded parser values instead of emitting invalid JSON");

        const Json nonFinite = std::numeric_limits<double>::quiet_NaN();
        errorMessage.clear();
        const std::optional<std::string> nonFiniteEncoding =
            ProtocolCodec::encodeSuccessResponse(integerResponseId, nonFinite, errorMessage);
        testResult.expectTrue(!nonFiniteEncoding && !errorMessage.empty(),
                              "encoder rejects non-finite numbers instead of silently changing them to null");
    }

    void testInitializationCodec(tests::support::TestResult& testResult) {
        ClientInfo clientInfo;
        clientInfo.name = "codec_test";
        clientInfo.title = "Codec Test";
        clientInfo.version = "9.8.7";

        const std::string initialize = ProtocolCodec::initializeRequest(401, clientInfo);
        testResult.expectTrue(!initialize.empty(), "initialize encoding produces a JSON document");
        testResult.expectTrue(initialize.find('\n') == std::string::npos && initialize.find('\r') == std::string::npos,
                              "initialize encoding contains no stdio line delimiter");

        std::string errorMessage;
        const std::optional<ProtocolMessage> initializeMessage = ProtocolCodec::decode(initialize, errorMessage);
        testResult.expectTrue(initializeMessage && initializeMessage->kind == ProtocolMessage::Kind::Request &&
                                  hasIntegerId(initializeMessage->id, 401) && initializeMessage->method == "initialize",
                              "initialize wrapper uses the generic correlated request encoder");
        testResult.expectTrue(initializeMessage && initializeMessage->params["clientInfo"]["name"] == "codec_test" &&
                                  initializeMessage->params["clientInfo"]["title"] == "Codec Test" &&
                                  initializeMessage->params["clientInfo"]["version"] == "9.8.7",
                              "initialize wrapper preserves all configured client information");

        const std::string initialized = ProtocolCodec::initializedNotification();
        testResult.expectTrue(!initialized.empty(), "initialized encoding produces a JSON document");
        testResult.expectTrue(initialized.find('\n') == std::string::npos && initialized.find('\r') == std::string::npos,
                              "initialized encoding contains no stdio line delimiter");
        const std::optional<ProtocolMessage> initializedMessage = ProtocolCodec::decode(initialized, errorMessage);
        testResult.expectTrue(initializedMessage && initializedMessage->kind == ProtocolMessage::Kind::Notification &&
                                  initializedMessage->method == "initialized" && initializedMessage->params == Json::object(),
                              "initialized wrapper uses the generic notification encoder");

        const Json rawInitializeResult = {
            {"codexHome", "/tmp/fake-codex"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "snodec-codec-test"},
            {"futureField", {{"nested", Json::array({1, 2, 3})}}},
        };
        errorMessage = "stale";
        const std::optional<InitializeResult> initializeResult =
            ai::openai::codex::detail::decodeInitializeResult(rawInitializeResult, errorMessage);
        testResult.expectTrue(initializeResult && initializeResult->codexHome == "/tmp/fake-codex" &&
                                  initializeResult->platformFamily == "unix" && initializeResult->platformOs == "linux" &&
                                  initializeResult->userAgent == "snodec-codec-test",
                              "separate initialization decoder caches all typed fields");
        testResult.expectTrue(initializeResult && initializeResult->raw == rawInitializeResult,
                              "separate initialization decoder preserves complete raw result and future fields");
        testResult.expectTrue(errorMessage.empty(), "successful initialization decoding clears a previous error");

        errorMessage.clear();
        const std::optional<InitializeResult> scalarInitializeResult =
            ai::openai::codex::detail::decodeInitializeResult(Json::array(), errorMessage);
        testResult.expectTrue(!scalarInitializeResult && !errorMessage.empty(),
                              "initialization decoder rejects a non-object result with a reason");

        Json missingFieldResult = rawInitializeResult;
        missingFieldResult.erase("userAgent");
        errorMessage.clear();
        const std::optional<InitializeResult> missingFieldInitializeResult =
            ai::openai::codex::detail::decodeInitializeResult(missingFieldResult, errorMessage);
        testResult.expectTrue(!missingFieldInitializeResult && !errorMessage.empty(),
                              "initialization decoder rejects a missing required typed field");

        Json invalidFieldResult = rawInitializeResult;
        invalidFieldResult["platformOs"] = Json::array();
        errorMessage.clear();
        const std::optional<InitializeResult> invalidFieldInitializeResult =
            ai::openai::codex::detail::decodeInitializeResult(invalidFieldResult, errorMessage);
        testResult.expectTrue(!invalidFieldInitializeResult && !errorMessage.empty(),
                              "initialization decoder rejects a required field with the wrong type");
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    testRequestsAndNotifications(testResult);
    testResultResponses(testResult);
    testRemoteErrors(testResult);
    testMalformedAndAmbiguousMessages(testResult);
    testGenericEncoders(testResult);
    testInitializationCodec(testResult);

    return testResult.processResult();
}
