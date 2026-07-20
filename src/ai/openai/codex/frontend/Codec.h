/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_FRONTEND_CODEC_H
#define AI_OPENAI_CODEX_FRONTEND_CODEC_H

#include "ai/openai/codex/frontend/Messages.h"

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::frontend {

    struct CodecError {
        ErrorCode code = ErrorCode::InvalidField;
        std::string message;
        bool closeConnection = false;
        std::vector<std::uint32_t> supportedVersions;
        std::optional<std::string> requestId;
        std::optional<Json> details;

        bool operator==(const CodecError&) const = default;
    };

    template <typename T>
    class CodecResult {
    public:
        CodecResult(T value)
            : result(std::move(value)) {
        }

        CodecResult(CodecError error)
            : result(std::move(error)) {
        }

        [[nodiscard]] bool hasValue() const noexcept {
            return std::holds_alternative<T>(result);
        }

        explicit operator bool() const noexcept {
            return hasValue();
        }

        [[nodiscard]] const T& value() const& {
            return std::get<T>(result);
        }

        [[nodiscard]] T&& value() && {
            return std::get<T>(std::move(result));
        }

        [[nodiscard]] const CodecError& error() const& {
            return std::get<CodecError>(result);
        }

        [[nodiscard]] CodecError&& error() && {
            return std::get<CodecError>(std::move(result));
        }

    private:
        std::variant<T, CodecError> result;
    };

    class Codec {
    public:
        [[nodiscard]] static CodecResult<ClientMessage> decodeClient(const Json& message) noexcept;
        [[nodiscard]] static CodecResult<ClientMessage> decodeClient(std::string_view message) noexcept;
        [[nodiscard]] static CodecResult<ServerMessage> decodeServer(const Json& message) noexcept;
        [[nodiscard]] static CodecResult<ServerMessage> decodeServer(std::string_view message) noexcept;

        [[nodiscard]] static CodecResult<Json> encodeClient(const ClientMessage& message) noexcept;
        [[nodiscard]] static CodecResult<Json> encodeServer(const ServerMessage& message) noexcept;
        [[nodiscard]] static CodecResult<std::string> serializeClient(const ClientMessage& message) noexcept;
        [[nodiscard]] static CodecResult<std::string> serializeServer(const ServerMessage& message) noexcept;

        // Event encoding excludes the surrounding event-batch envelope. It is
        // public so EventJournal can enforce an exact retained-byte bound.
        [[nodiscard]] static CodecResult<Json> encodeEvent(const FrontendEvent& event) noexcept;
        [[nodiscard]] static CodecResult<std::size_t> serializedEventSize(const FrontendEvent& event) noexcept;
    };

} // namespace ai::openai::codex::frontend

#endif // AI_OPENAI_CODEX_FRONTEND_CODEC_H
