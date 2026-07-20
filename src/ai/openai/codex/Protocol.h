/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_PROTOCOL_H
#define AI_OPENAI_CODEX_PROTOCOL_H

#include <compare>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace ai::openai::codex {

    using Json = nlohmann::json;

    struct Error {
        enum class Category { Launch, Transport, Protocol, Initialization, Process, InvalidState, Capacity, Cancelled, Enqueue };

        Category category = Category::Transport;
        int code = 0;
        std::string message;
    };

    class ClientRequestId {
    public:
        explicit ClientRequestId(std::int64_t value)
            : id(value) {
        }

        std::int64_t value() const noexcept {
            return id;
        }

        auto operator<=>(const ClientRequestId&) const = default;

    private:
        std::int64_t id;
    };

    class ServerRequestId {
    public:
        using Value = std::variant<std::int64_t, std::string>;

        explicit ServerRequestId(std::int64_t value)
            : id(value) {
        }

        explicit ServerRequestId(std::string value)
            : id(std::move(value)) {
        }

        explicit ServerRequestId(Value value)
            : id(std::move(value)) {
        }

        const Value& value() const noexcept {
            return id;
        }

        auto operator<=>(const ServerRequestId&) const = default;

    private:
        Value id;
    };

    class ServerRequestToken {
    public:
        ServerRequestToken() = default;

        explicit ServerRequestToken(std::uint64_t value)
            : token(value) {
        }

        std::uint64_t value() const noexcept {
            return token;
        }

        explicit operator bool() const noexcept {
            return token != 0;
        }

        auto operator<=>(const ServerRequestToken&) const = default;

    private:
        std::uint64_t token = 0;
    };

    struct ProtocolError {
        std::int64_t code = 0;
        std::string message;
        std::optional<Json> data;
    };

    struct InitializeResult {
        std::string codexHome;
        std::string platformFamily;
        std::string platformOs;
        std::string userAgent;

        Json raw;
    };

    struct Response {
        enum class Kind { Result, RemoteError, Cancelled };

        ClientRequestId id;
        std::string method;
        Kind kind = Kind::Result;

        Json result = nullptr;
        std::optional<ProtocolError> remoteError;
        std::optional<Error> localError;
    };

    struct Notification {
        std::string method;
        Json params = nullptr;
        Json raw;
    };

    struct ServerRequest {
        ServerRequestId id;
        std::string method;
        Json params = nullptr;
        Json raw;
        ServerRequestToken token;
    };

    struct UnknownMessage {
        Json raw;
        std::string reason;
    };

} // namespace ai::openai::codex

#endif // AI_OPENAI_CODEX_PROTOCOL_H
