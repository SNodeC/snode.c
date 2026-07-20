/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_PROTOCOLCODEC_H
#define AI_OPENAI_CODEX_DETAIL_PROTOCOLCODEC_H

#include "ai/openai/codex/Protocol.h"

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace ai::openai::codex {
    struct ClientInfo;
}

namespace ai::openai::codex::detail {

    using ProtocolId = std::variant<std::int64_t, std::string>;

    struct ProtocolMessage {
        enum class Kind { Response, Request, Notification };

        Kind kind = Kind::Notification;
        std::optional<ProtocolId> id;
        std::string method;
        Json params = nullptr;
        Json result = nullptr;
        std::optional<ProtocolError> error;
        Json raw;
    };

    std::optional<InitializeResult> decodeInitializeResult(const Json& result, std::string& errorMessage);

    class ProtocolCodec {
    public:
        ProtocolCodec() = delete;

        static std::optional<std::string>
        encodeRequest(std::int64_t id, std::string_view method, const Json& params, std::string& errorMessage);
        static std::optional<std::string> encodeNotification(std::string_view method, const Json& params, std::string& errorMessage);
        static std::optional<std::string> encodeSuccessResponse(const ProtocolId& id, const Json& result, std::string& errorMessage);
        static std::optional<std::string> encodeErrorResponse(const ProtocolId& id, const ProtocolError& error, std::string& errorMessage);

        static std::string initializeRequest(std::int64_t id, const ClientInfo& clientInfo);
        static std::string initializedNotification();

        static std::optional<ProtocolMessage> decode(std::string_view wireMessage, std::string& errorMessage);
    };

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_PROTOCOLCODEC_H
