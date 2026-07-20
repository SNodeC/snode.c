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

#include <string>

int main() {
    tests::support::TestResult testResult;

    ai::openai::codex::ClientInfo clientInfo;
    clientInfo.name = "codec_test";
    const std::string initialize = ai::openai::codex::detail::ProtocolCodec::initializeRequest(42, clientInfo);
    const std::string initialized = ai::openai::codex::detail::ProtocolCodec::initializedNotification();

    testResult.expectTrue(!initialize.empty(), "initialize encoding produces a JSON document");
    testResult.expectTrue(initialize.find('\n') == std::string::npos && initialize.find('\r') == std::string::npos,
                          "initialize encoding contains no stdio line delimiter");
    testResult.expectTrue(!initialized.empty(), "initialized encoding produces a JSON document");
    testResult.expectTrue(initialized.find('\n') == std::string::npos && initialized.find('\r') == std::string::npos,
                          "initialized encoding contains no stdio line delimiter");

    std::string decodeError;
    const auto initializeMessage = ai::openai::codex::detail::ProtocolCodec::decode(initialize, decodeError);
    testResult.expectTrue(initializeMessage && initializeMessage->kind == ai::openai::codex::detail::ProtocolMessage::Kind::Request &&
                              initializeMessage->id == 42 && initializeMessage->method == "initialize",
                          "initialize encoding remains a valid correlated protocol request");

    decodeError.clear();
    const auto initializedMessage = ai::openai::codex::detail::ProtocolCodec::decode(initialized, decodeError);
    testResult.expectTrue(initializedMessage &&
                              initializedMessage->kind == ai::openai::codex::detail::ProtocolMessage::Kind::Notification &&
                              initializedMessage->method == "initialized",
                          "initialized encoding remains a valid protocol notification");

    return testResult.processResult();
}
