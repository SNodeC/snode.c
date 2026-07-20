/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include <ai/openai/codex/Protocol.h>
#include <ai/openai/codex/stdio/Client.h>
#include <optional>
#include <string>

int main() {
    using namespace ai::openai::codex;

    stdio::Client client;
    AppServerClient::RawProtocol& protocol = client.raw();

    protocol.setOnNotification([](const Notification&) {
    });
    protocol.setOnServerRequest([](const ServerRequest&) {
    });
    protocol.setOnUnknownMessage([](const UnknownMessage&) {
    });

    const AppServerClient::RawProtocol::Submission request = protocol.request("thread/start", Json::object(), [](const Response&) {
    });
    const AppServerClient::RawProtocol::SendResult notification = protocol.notify("client/example", Json::object());

    const ServerRequestId integerId(7);
    const AppServerClient::RawProtocol::SendResult response = protocol.respond(integerId, Json{{"decision", "accept"}});

    const ServerRequestId stringId(std::string("request-7"));
    const AppServerClient::RawProtocol::SendResult rejection =
        protocol.reject(stringId, ProtocolError{.code = -32000, .message = "Request rejected", .data = std::nullopt});

    const std::optional<InitializeResult> initializeResult = client.getInitializeResult();

    return request || notification || response || rejection || initializeResult ? 1 : 0;
}
