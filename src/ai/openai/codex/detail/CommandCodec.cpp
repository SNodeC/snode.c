/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/CommandCodec.h"

#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/typed/Types.h"

#include <cstdint>
#include <limits>
#include <map>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {
        bool fail(std::string& error, std::string message) {
            error = std::move(message);
            return false;
        }

        const Json* member(const Json& object, std::string_view name) noexcept {
            if (!object.is_object()) {
                return nullptr;
            }
            const auto iterator = object.find(name);
            return iterator == object.end() ? nullptr : &*iterator;
        }

        template <typename T, typename Encode>
        bool encodeOptionalNullable(
            Json& object, std::string_view name, const typed::OptionalNullable<T>& value, Encode&& encode, std::string& error) {
            if (!value.present && value.value.has_value()) {
                return fail(error, "command parameter field '$." + std::string(name) + "' has an inconsistent omitted state");
            }
            if (value.isOmitted()) {
                return true;
            }
            if (value.isNull()) {
                object[std::string(name)] = nullptr;
                return true;
            }
            std::optional<Json> encoded = encode(*value, error);
            if (!encoded) {
                return false;
            }
            object[std::string(name)] = std::move(*encoded);
            return true;
        }

        template <typename T>
        void encodeOptional(Json& object, std::string_view name, const std::optional<T>& value) {
            if (value) {
                object[std::string(name)] = *value;
            }
        }

        std::optional<Json> encodeString(const std::string& value, std::string&) {
            return std::optional<Json>{Json(value)};
        }

        std::optional<Json> encodeProcessId(const typed::CommandExecProcessId& value, std::string&) {
            return std::optional<Json>{Json(value.value)};
        }

        std::optional<Json> encodeTerminalSize(const typed::CommandExecTerminalSize& value, std::string&) {
            return std::optional<Json>{Json{{"cols", value.cols}, {"rows", value.rows}}};
        }

        std::optional<Json> encodeEnvironment(const std::map<std::string, std::optional<std::string>>& value, std::string&) {
            Json result = Json::object();
            for (const auto& [name, entry] : value) {
                result[name] = entry ? Json(*entry) : Json(nullptr);
            }
            return std::optional<Json>{std::move(result)};
        }

        std::optional<Json> encodeUint64(const std::uint64_t& value, std::string&) {
            return std::optional<Json>{Json(value)};
        }

        std::optional<Json> encodeInt64(const std::int64_t& value, std::string&) {
            return std::optional<Json>{Json(value)};
        }

        bool decodeInt32(const Json& value, std::int32_t& output) noexcept {
            if (value.is_number_unsigned()) {
                const auto number = value.get_ref<const Json::number_unsigned_t&>();
                if (number > static_cast<Json::number_unsigned_t>(std::numeric_limits<std::int32_t>::max())) {
                    return false;
                }
                output = static_cast<std::int32_t>(number);
                return true;
            }
            if (!value.is_number_integer()) {
                return false;
            }
            const auto number = value.get_ref<const Json::number_integer_t&>();
            if (number < std::numeric_limits<std::int32_t>::min() || number > std::numeric_limits<std::int32_t>::max()) {
                return false;
            }
            output = static_cast<std::int32_t>(number);
            return true;
        }

        bool requireString(const Json& object, std::string_view name, std::string& output, std::string& error, std::string_view context) {
            const Json* value = member(object, name);
            if (value == nullptr || !value->is_string()) {
                return fail(error, std::string(context) + " field '$." + std::string(name) + "' must be a string");
            }
            output = value->get_ref<const std::string&>();
            return true;
        }

        bool requireBoolean(const Json& object, std::string_view name, bool& output, std::string& error, std::string_view context) {
            const Json* value = member(object, name);
            if (value == nullptr || !value->is_boolean()) {
                return fail(error, std::string(context) + " field '$." + std::string(name) + "' must be a boolean");
            }
            output = value->get_ref<const Json::boolean_t&>();
            return true;
        }
    } // namespace

    std::optional<Json> encodeCommandExecParams(const typed::CommandExecParams& value, std::string& error) noexcept {
        try {
            if (value.command.empty()) {
                fail(error, "command/exec field '$.command' must contain at least one argv element");
                return std::nullopt;
            }
            Json result{{"command", value.command}};
            encodeOptional(result, "disableOutputCap", value.disableOutputCap);
            encodeOptional(result, "disableTimeout", value.disableTimeout);
            encodeOptional(result, "streamStdin", value.streamStdin);
            encodeOptional(result, "streamStdoutStderr", value.streamStdoutStderr);
            encodeOptional(result, "tty", value.tty);
            if (!encodeOptionalNullable(result, "cwd", value.cwd, encodeString, error) ||
                !encodeOptionalNullable(result, "env", value.env, encodeEnvironment, error) ||
                !encodeOptionalNullable(result, "outputBytesCap", value.outputBytesCap, encodeUint64, error) ||
                !encodeOptionalNullable(result, "processId", value.processId, encodeProcessId, error) ||
                !encodeOptionalNullable(result, "sandboxPolicy", value.sandboxPolicy, encodeSandboxPolicy, error) ||
                !encodeOptionalNullable(result, "size", value.size, encodeTerminalSize, error) ||
                !encodeOptionalNullable(result, "timeoutMs", value.timeoutMs, encodeInt64, error)) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            fail(error, "command/exec parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeCommandExecResizeParams(const typed::CommandExecResizeParams& value, std::string& error) noexcept {
        try {
            std::optional<Json> processId = encodeProcessId(value.processId, error);
            if (!processId) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{
                Json{{"processId", std::move(*processId)}, {"size", {{"cols", value.size.cols}, {"rows", value.size.rows}}}}};
        } catch (...) {
            fail(error, "command/exec/resize parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeCommandExecTerminateParams(const typed::CommandExecTerminateParams& value, std::string& error) noexcept {
        try {
            std::optional<Json> processId = encodeProcessId(value.processId, error);
            if (!processId) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{Json{{"processId", std::move(*processId)}}};
        } catch (...) {
            fail(error, "command/exec/terminate parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeCommandExecWriteParams(const typed::CommandExecWriteParams& value, std::string& error) noexcept {
        try {
            std::optional<Json> processId = encodeProcessId(value.processId, error);
            if (!processId) {
                return std::nullopt;
            }
            Json result{{"processId", std::move(*processId)}};
            encodeOptional(result, "closeStdin", value.closeStdin);
            if (!encodeOptionalNullable(result, "deltaBase64", value.deltaBase64, encodeString, error)) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            fail(error, "command/exec/write parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<typed::CommandExecResponse> decodeCommandExecResponse(const Json& value, std::string& error) noexcept {
        try {
            if (!value.is_object()) {
                fail(error, "CommandExecResponse at '$' must be an object");
                return std::nullopt;
            }
            typed::CommandExecResponse result;
            const Json* exitCode = member(value, "exitCode");
            if (exitCode == nullptr || !decodeInt32(*exitCode, result.exitCode)) {
                fail(error, "CommandExecResponse field '$.exitCode' must be a signed 32-bit integer");
                return std::nullopt;
            }
            if (!requireString(value, "stdout", result.stdoutData, error, "CommandExecResponse") ||
                !requireString(value, "stderr", result.stderrData, error, "CommandExecResponse")) {
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "CommandExecResponse could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::CommandExecOutputDeltaNotification> decodeCommandExecOutputDeltaNotification(const Notification& notification,
                                                                                                      std::string& error) noexcept {
        try {
            if (!notification.params.is_object()) {
                fail(error, "command/exec/outputDelta params at '$.params' must be an object");
                return std::nullopt;
            }
            typed::CommandExecOutputDeltaNotification result;
            std::string processId;
            if (!requireBoolean(notification.params, "capReached", result.capReached, error, "command/exec/outputDelta") ||
                !requireString(notification.params, "deltaBase64", result.deltaBase64, error, "command/exec/outputDelta") ||
                !requireString(notification.params, "processId", processId, error, "command/exec/outputDelta") ||
                !requireString(notification.params, "stream", result.stream.value, error, "command/exec/outputDelta")) {
                return std::nullopt;
            }
            result.processId.value = std::move(processId);
            if (!result.stream.isKnown()) {
                result.diagnostics.push_back({typed::DecodeIssueKind::UnknownEnumValue,
                                              typed::DecodeIssueSeverity::ForwardCompatibility,
                                              "CommandExecOutputStream",
                                              "$.params.stream",
                                              "protocol surface contains a future string-enum value"});
            }
            result.raw = notification.raw;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "command/exec/outputDelta could not be decoded");
            return std::nullopt;
        }
    }

} // namespace ai::openai::codex::detail
