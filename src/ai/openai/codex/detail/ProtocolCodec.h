/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_PROTOCOLCODEC_H
#define AI_OPENAI_CODEX_DETAIL_PROTOCOLCODEC_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ai::openai::codex {
    struct ClientInfo;
}

namespace ai::openai::codex::detail {

    struct ProtocolError {
        int code = 0;
        std::string message;
    };

    struct InitializeResult {
        std::string codexHome;
        std::string platformFamily;
        std::string platformOs;
        std::string userAgent;
    };

    struct ProtocolMessage {
        enum class Kind { Response, Request, Notification };

        Kind kind = Kind::Notification;
        std::optional<std::int64_t> id;
        std::string method;
        std::optional<ProtocolError> error;
        bool hasResult = false;
        std::optional<InitializeResult> initializeResult;
        std::string resultError;
    };

    class ProtocolCodec {
    public:
        ProtocolCodec() = delete;

        static std::string initializeRequest(std::int64_t id, const ClientInfo& clientInfo);
        static std::string initializedNotification();

        static std::optional<ProtocolMessage> decode(std::string_view wireMessage, std::string& errorMessage);
    };

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_PROTOCOLCODEC_H
