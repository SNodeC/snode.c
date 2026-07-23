/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_TYPES_H
#define AI_OPENAI_CODEX_TYPED_TYPES_H

#include "ai/openai/codex/Protocol.h"

#include <compare>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>

namespace ai::openai::codex::typed {

    enum class DecodeIssueKind { UnknownMethod, UnknownDiscriminator, UnknownEnumValue, MalformedKnownPayload };

    enum class DecodeIssueSeverity { ForwardCompatibility, ProtocolWarning };

    struct DecodeDiagnostic {
        DecodeIssueKind kind = DecodeIssueKind::MalformedKnownPayload;
        DecodeIssueSeverity severity = DecodeIssueSeverity::ProtocolWarning;
        std::string surface;
        std::string fieldPath;
        std::string message;

        auto operator<=>(const DecodeDiagnostic&) const = default;
    };

    // Represents the three distinct wire states of an optional nullable field:
    // omitted, present with null, and present with a value.
    template <typename T>
    struct OptionalNullable {
        bool present = false;
        std::optional<T> value;

        constexpr OptionalNullable() noexcept = default;

        // Compatibility with the former optional<T> fields: nullopt means
        // omitted, while a populated optional means present with a value.
        constexpr OptionalNullable(std::nullopt_t) noexcept {
        }

        constexpr OptionalNullable(const T& input)
            : present(true)
            , value(input) {
        }

        constexpr OptionalNullable(T&& input)
            : present(true)
            , value(std::move(input)) {
        }

        constexpr OptionalNullable(const std::optional<T>& input)
            : present(input.has_value())
            , value(input) {
        }

        constexpr OptionalNullable(std::optional<T>&& input)
            : present(input.has_value())
            , value(std::move(input)) {
        }

        // Retains source compatibility with the original aggregate's
        // OptionalNullable{present, value} spelling.
        constexpr OptionalNullable(bool isPresent, std::optional<T> input)
            : present(isPresent)
            , value(std::move(input)) {
        }

        [[nodiscard]] static constexpr OptionalNullable omitted() noexcept {
            return {};
        }

        [[nodiscard]] static constexpr OptionalNullable explicitNull() noexcept {
            OptionalNullable result;
            result.present = true;
            return result;
        }

        [[nodiscard]] static constexpr OptionalNullable withValue(T input) {
            return OptionalNullable{std::move(input)};
        }

        [[nodiscard]] constexpr bool isOmitted() const noexcept {
            return !present;
        }

        [[nodiscard]] constexpr bool isNull() const noexcept {
            return present && !value.has_value();
        }

        [[nodiscard]] constexpr bool hasValue() const noexcept {
            return present && value.has_value();
        }

        // std::optional-like compatibility for callers that only need to know
        // whether a value exists. Use isOmitted()/isNull() when wire state
        // matters.
        [[nodiscard]] constexpr bool has_value() const noexcept {
            return hasValue();
        }

        constexpr explicit operator bool() const noexcept {
            return hasValue();
        }

        constexpr T& operator*() & {
            return *value;
        }

        constexpr const T& operator*() const& {
            return *value;
        }

        constexpr T&& operator*() && {
            return *std::move(value);
        }

        constexpr T* operator->() {
            return std::addressof(*value);
        }

        constexpr const T* operator->() const {
            return std::addressof(*value);
        }

        constexpr OptionalNullable& operator=(std::nullopt_t) noexcept {
            present = false;
            value.reset();
            return *this;
        }

        constexpr OptionalNullable& operator=(const T& input) {
            present = true;
            value = input;
            return *this;
        }

        constexpr OptionalNullable& operator=(T&& input) {
            present = true;
            value = std::move(input);
            return *this;
        }

        constexpr OptionalNullable& operator=(const std::optional<T>& input) {
            present = input.has_value();
            value = input;
            return *this;
        }

        constexpr OptionalNullable& operator=(std::optional<T>&& input) {
            present = input.has_value();
            value = std::move(input);
            return *this;
        }

        auto operator<=>(const OptionalNullable&) const = default;
    };

    struct AbsolutePathBuf {
        std::string value;

        AbsolutePathBuf() = default;

        // Intentional implicit compatibility conversion: the earlier public
        // sandbox aggregate accepted vector<string> roots, while the canonical
        // protocol type is now strong.
        AbsolutePathBuf(std::string input)
            : value(std::move(input)) {
        }

        auto operator<=>(const AbsolutePathBuf&) const = default;
    };

    struct ByteRange {
        std::uint64_t start = 0;
        std::uint64_t end = 0;

        auto operator<=>(const ByteRange&) const = default;
    };

    struct TextElement {
        ByteRange byteRange;
        OptionalNullable<std::string> placeholder;

        auto operator<=>(const TextElement&) const = default;
    };

    struct ThreadId {
        std::string value;

        auto operator<=>(const ThreadId&) const = default;
    };

    struct TurnId {
        std::string value;

        auto operator<=>(const TurnId&) const = default;
    };

    struct ClientUserMessageId {
        std::string value;

        auto operator<=>(const ClientUserMessageId&) const = default;
    };

    struct ItemId {
        std::string value;

        auto operator<=>(const ItemId&) const = default;
    };

    struct ModelId {
        std::string value;

        auto operator<=>(const ModelId&) const = default;
    };

    struct ResponseCallId {
        std::string value;

        auto operator<=>(const ResponseCallId&) const = default;
    };

    struct ResponseItemId {
        std::string value;

        auto operator<=>(const ResponseItemId&) const = default;
    };

    struct ReasoningEffort {
        std::string value;

        static ReasoningEffort minimal();
        static ReasoningEffort low();
        static ReasoningEffort medium();
        static ReasoningEffort high();
        static ReasoningEffort xhigh();

        auto operator<=>(const ReasoningEffort&) const = default;
    };

    struct ApprovalPolicy {
        std::string value;

        static ApprovalPolicy untrusted();
        static ApprovalPolicy onRequest();
        static ApprovalPolicy never();

        auto operator<=>(const ApprovalPolicy&) const = default;
    };

    struct SandboxMode {
        std::string value;

        static SandboxMode readOnly();
        static SandboxMode workspaceWrite();
        static SandboxMode dangerFullAccess();

        auto operator<=>(const SandboxMode&) const = default;
    };

    struct ImageDetail {
        std::string value;

        static ImageDetail automatic();
        static ImageDetail low();
        static ImageDetail high();
        static ImageDetail original();

        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const ImageDetail&) const = default;
    };

    struct NetworkAccess {
        std::string value;

        static NetworkAccess restricted();
        static NetworkAccess enabled();

        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const NetworkAccess&) const = default;
    };

    struct ThreadStatus {
        std::string value;
        Json raw = Json::object();
    };

    struct TurnStatus {
        std::string value;

        static TurnStatus completed();
        static TurnStatus interrupted();
        static TurnStatus failed();
        static TurnStatus inProgress();

        auto operator<=>(const TurnStatus&) const = default;
    };

    struct TurnItemsView {
        std::string value;

        static TurnItemsView notLoaded();
        static TurnItemsView summary();
        static TurnItemsView full();

        auto operator<=>(const TurnItemsView&) const = default;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_TYPES_H
