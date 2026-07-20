/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ThreadCodec.h"

#include "ai/openai/codex/detail/TurnCodec.h"

#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {

        const Json* findMember(const Json& object, const std::string_view name) noexcept {
            if (!object.is_object()) {
                return nullptr;
            }

            const auto member = object.find(name);

            return member == object.end() ? nullptr : &*member;
        }

        bool fail(std::string& error, std::string message) {
            error = std::move(message);

            return false;
        }

        bool decodeInt64(const Json& value, std::int64_t& result) noexcept {
            if (value.is_number_unsigned()) {
                const auto raw = value.get_ref<const Json::number_unsigned_t&>();

                if (raw > static_cast<Json::number_unsigned_t>(std::numeric_limits<std::int64_t>::max())) {
                    return false;
                }

                result = static_cast<std::int64_t>(raw);
                return true;
            }

            if (!value.is_number_integer()) {
                return false;
            }

            result = value.get_ref<const Json::number_integer_t&>();
            return true;
        }

        bool decodeRequiredString(const Json& object, const std::string_view name, std::string& result, std::string& error) {
            const Json* value = findMember(object, name);

            if (value == nullptr) {
                return fail(error, "thread is missing required field '" + std::string(name) + "'");
            }
            if (!value->is_string()) {
                return fail(error, "thread field '" + std::string(name) + "' must be a string");
            }

            result = value->get_ref<const std::string&>();
            return true;
        }

        bool decodeRequiredInt64(const Json& object, const std::string_view name, std::int64_t& result, std::string& error) {
            const Json* value = findMember(object, name);

            if (value == nullptr) {
                return fail(error, "thread is missing required field '" + std::string(name) + "'");
            }
            if (!decodeInt64(*value, result)) {
                return fail(error, "thread field '" + std::string(name) + "' must be an int64 integer");
            }

            return true;
        }

        bool decodeRequiredBool(const Json& object, const std::string_view name, bool& result, std::string& error) {
            const Json* value = findMember(object, name);

            if (value == nullptr) {
                return fail(error, "thread is missing required field '" + std::string(name) + "'");
            }
            if (!value->is_boolean()) {
                return fail(error, "thread field '" + std::string(name) + "' must be a boolean");
            }

            result = value->get_ref<const Json::boolean_t&>();
            return true;
        }

        bool decodeOptionalString(const Json& object, const std::string_view name, std::optional<std::string>& result, std::string& error) {
            const Json* value = findMember(object, name);

            if (value == nullptr || value->is_null()) {
                result.reset();
                return true;
            }
            if (!value->is_string()) {
                return fail(error, "thread field '" + std::string(name) + "' must be a string or null");
            }

            result = value->get_ref<const std::string&>();
            return true;
        }

        bool validateThreadSource(const Json& object, std::string& error) {
            const Json* source = findMember(object, "source");

            if (source == nullptr) {
                return fail(error, "thread is missing required field 'source'");
            }
            if (source->is_string()) {
                return true;
            }
            if (!source->is_object() || source->empty()) {
                return fail(error, "thread field 'source' must be a string or a non-empty object");
            }

            const Json* custom = findMember(*source, "custom");
            if (custom != nullptr && !custom->is_string()) {
                return fail(error, "thread field 'source.custom' must be a string");
            }

            const Json* subAgent = findMember(*source, "subAgent");
            if (subAgent != nullptr && !subAgent->is_string() && (!subAgent->is_object() || subAgent->empty())) {
                return fail(error, "thread field 'source.subAgent' must be a string or a non-empty object");
            }

            // Unknown object variants are deliberately accepted for forward
            // compatibility. Their complete representation remains in Thread::raw.
            return true;
        }

        bool decodeThreadStatus(const Json& object, typed::ThreadStatus& result, std::string& error) {
            const Json* status = findMember(object, "status");

            if (status == nullptr) {
                return fail(error, "thread is missing required field 'status'");
            }
            if (!status->is_object()) {
                return fail(error, "thread field 'status' must be an object");
            }

            const Json* type = findMember(*status, "type");
            if (type == nullptr || !type->is_string()) {
                return fail(error, "thread field 'status.type' must be a string");
            }

            const std::string& typeValue = type->get_ref<const std::string&>();
            if (typeValue == "active") {
                const Json* activeFlags = findMember(*status, "activeFlags");
                if (activeFlags == nullptr || !activeFlags->is_array()) {
                    return fail(error, "active thread status requires an 'activeFlags' array");
                }
            }

            result.value = typeValue;
            result.raw = *status;
            return true;
        }

        bool validateThreadOperationWrapper(const Json& value, std::string& error) {
            if (!value.is_object()) {
                return fail(error, "thread operation result must be an object");
            }

            const Json* approvalPolicy = findMember(value, "approvalPolicy");
            if (approvalPolicy == nullptr) {
                return fail(error, "thread operation result is missing required field 'approvalPolicy'");
            }
            if (!approvalPolicy->is_string() && !approvalPolicy->is_object()) {
                return fail(error, "thread operation result field 'approvalPolicy' must be a string or object");
            }

            const Json* approvalsReviewer = findMember(value, "approvalsReviewer");
            if (approvalsReviewer == nullptr || !approvalsReviewer->is_string()) {
                return fail(error, "thread operation result field 'approvalsReviewer' must be a string");
            }

            const Json* cwd = findMember(value, "cwd");
            if (cwd == nullptr || !cwd->is_string()) {
                return fail(error, "thread operation result field 'cwd' must be a string");
            }

            const Json* model = findMember(value, "model");
            if (model == nullptr || !model->is_string()) {
                return fail(error, "thread operation result field 'model' must be a string");
            }

            const Json* modelProvider = findMember(value, "modelProvider");
            if (modelProvider == nullptr || !modelProvider->is_string()) {
                return fail(error, "thread operation result field 'modelProvider' must be a string");
            }

            const Json* sandbox = findMember(value, "sandbox");
            if (sandbox == nullptr || !sandbox->is_object()) {
                return fail(error, "thread operation result field 'sandbox' must be an object");
            }
            const Json* sandboxType = findMember(*sandbox, "type");
            if (sandboxType == nullptr || !sandboxType->is_string()) {
                return fail(error, "thread operation result field 'sandbox.type' must be a string");
            }

            const Json* thread = findMember(value, "thread");
            if (thread == nullptr || !thread->is_object()) {
                return fail(error, "thread operation result field 'thread' must be an object");
            }

            return true;
        }

        void addThreadSettings(Json& params,
                               const std::optional<std::string>& cwd,
                               const std::optional<typed::ModelId>& model,
                               const std::optional<std::string>& modelProvider,
                               const std::optional<typed::ApprovalPolicy>& approvalPolicy,
                               const std::optional<typed::SandboxMode>& sandboxMode) {
            if (cwd.has_value()) {
                params.emplace("cwd", *cwd);
            }
            if (model.has_value()) {
                params.emplace("model", model->value);
            }
            if (modelProvider.has_value()) {
                params.emplace("modelProvider", *modelProvider);
            }
            if (approvalPolicy.has_value()) {
                params.emplace("approvalPolicy", approvalPolicy->value);
            }
            if (sandboxMode.has_value()) {
                params.emplace("sandbox", sandboxMode->value);
            }
        }

    } // namespace

    std::optional<Json> encodeThreadStartParams(const typed::ThreadStartOptions& options, std::string& error) {
        try {
            error.clear();
            Json params = Json::object();
            addThreadSettings(params, options.cwd, options.model, options.modelProvider, options.approvalPolicy, options.sandboxMode);

            if (options.ephemeral.has_value()) {
                params.emplace("ephemeral", *options.ephemeral);
            }

            return std::optional<Json>{std::move(params)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/start parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/start parameters: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<Json>
    encodeThreadResumeParams(const typed::ThreadId& threadId, const typed::ThreadResumeOptions& options, std::string& error) {
        try {
            error.clear();
            Json params = Json::object({{"threadId", threadId.value}});
            addThreadSettings(params, options.cwd, options.model, options.modelProvider, options.approvalPolicy, options.sandboxMode);

            return std::optional<Json>{std::move(params)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/resume parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/resume parameters: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<Json> encodeThreadListParams(const typed::ThreadListOptions& options, std::string& error) {
        try {
            error.clear();
            Json params = Json::object();

            if (options.cursor.has_value()) {
                params.emplace("cursor", *options.cursor);
            }
            if (options.limit.has_value()) {
                params.emplace("limit", *options.limit);
            }
            if (options.archived.has_value()) {
                params.emplace("archived", *options.archived);
            }
            if (options.searchTerm.has_value()) {
                params.emplace("searchTerm", *options.searchTerm);
            }

            return std::optional<Json>{std::move(params)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/list parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/list parameters: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<Json>
    encodeThreadReadParams(const typed::ThreadId& threadId, const typed::ThreadReadOptions& options, std::string& error) {
        try {
            error.clear();
            Json params = Json::object({{"threadId", threadId.value}});

            if (options.includeTurns.has_value()) {
                params.emplace("includeTurns", *options.includeTurns);
            }

            return std::optional<Json>{std::move(params)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/read parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/read parameters: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<typed::Thread> decodeThread(const Json& value, std::string& error, const std::optional<typed::ModelId> model) {
        try {
            error.clear();

            if (!value.is_object()) {
                fail(error, "thread must be an object");
                return std::nullopt;
            }

            typed::Thread result;
            std::string id;
            std::string cwd;
            std::string modelProvider;
            std::string preview;
            std::string cliVersion;
            std::string sessionId;
            std::int64_t createdAt = 0;
            std::int64_t updatedAt = 0;
            bool ephemeral = false;
            typed::ThreadStatus status;

            if (!decodeRequiredString(value, "id", id, error) || !decodeRequiredString(value, "cwd", cwd, error) ||
                !decodeRequiredString(value, "modelProvider", modelProvider, error) ||
                !decodeRequiredString(value, "preview", preview, error) || !decodeRequiredString(value, "cliVersion", cliVersion, error) ||
                !decodeRequiredString(value, "sessionId", sessionId, error) || !decodeRequiredInt64(value, "createdAt", createdAt, error) ||
                !decodeRequiredInt64(value, "updatedAt", updatedAt, error) || !decodeRequiredBool(value, "ephemeral", ephemeral, error) ||
                !validateThreadSource(value, error) || !decodeThreadStatus(value, status, error) ||
                !decodeOptionalString(value, "name", result.title, error)) {
                return std::nullopt;
            }

            const Json* turns = findMember(value, "turns");
            if (turns == nullptr) {
                fail(error, "thread is missing required field 'turns'");
                return std::nullopt;
            }
            if (!turns->is_array()) {
                fail(error, "thread field 'turns' must be an array");
                return std::nullopt;
            }

            result.id = typed::ThreadId{std::move(id)};
            result.cwd = std::move(cwd);
            result.model = model;
            result.modelProvider = std::move(modelProvider);
            result.preview = std::move(preview);
            result.cliVersion = std::move(cliVersion);
            result.sessionId = std::move(sessionId);
            result.status = std::move(status);
            result.ephemeral = ephemeral;
            result.createdAt = createdAt;
            result.updatedAt = updatedAt;
            result.turns.reserve(turns->size());

            std::size_t index = 0;
            for (const Json& turnValue : *turns) {
                std::string turnError;
                std::optional<typed::Turn> turn = decodeTurn(turnValue, result.id, turnError);

                if (!turn.has_value()) {
                    fail(error, "thread field 'turns'[" + std::to_string(index) + "]: " + turnError);
                    return std::nullopt;
                }

                result.turns.emplace_back(std::move(*turn));
                ++index;
            }

            result.raw = value;
            return result;
        } catch (const std::exception& exception) {
            error = std::string("failed to decode thread: ") + exception.what();
        } catch (...) {
            error = "failed to decode thread: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<typed::Thread> decodeThreadOperationResult(const Json& value, std::string& error) {
        try {
            error.clear();

            if (!validateThreadOperationWrapper(value, error)) {
                return std::nullopt;
            }

            const Json* model = findMember(value, "model");
            const Json* thread = findMember(value, "thread");

            // validateThreadOperationWrapper already established both fields and
            // their types. Keep the guards to avoid unchecked assumptions if that
            // helper changes independently.
            if (model == nullptr || thread == nullptr || !model->is_string() || !thread->is_object()) {
                fail(error, "thread operation result is internally inconsistent");
                return std::nullopt;
            }

            return decodeThread(*thread, error, typed::ModelId{model->get_ref<const std::string&>()});
        } catch (const std::exception& exception) {
            error = std::string("failed to decode thread operation result: ") + exception.what();
        } catch (...) {
            error = "failed to decode thread operation result: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<typed::ThreadPage> decodeThreadListResult(const Json& value, std::string& error) {
        try {
            error.clear();

            if (!value.is_object()) {
                fail(error, "thread/list result must be an object");
                return std::nullopt;
            }

            const Json* data = findMember(value, "data");
            if (data == nullptr) {
                fail(error, "thread/list result is missing required field 'data'");
                return std::nullopt;
            }
            if (!data->is_array()) {
                fail(error, "thread/list result field 'data' must be an array");
                return std::nullopt;
            }

            typed::ThreadPage result;
            if (!decodeOptionalString(value, "nextCursor", result.nextCursor, error) ||
                !decodeOptionalString(value, "backwardsCursor", result.backwardsCursor, error)) {
                return std::nullopt;
            }

            result.data.reserve(data->size());
            std::size_t index = 0;
            for (const Json& threadValue : *data) {
                std::string threadError;
                std::optional<typed::Thread> thread = decodeThread(threadValue, threadError);

                if (!thread.has_value()) {
                    fail(error, "thread/list result field 'data'[" + std::to_string(index) + "]: " + threadError);
                    return std::nullopt;
                }

                result.data.emplace_back(std::move(*thread));
                ++index;
            }

            result.raw = value;
            return result;
        } catch (const std::exception& exception) {
            error = std::string("failed to decode thread/list result: ") + exception.what();
        } catch (...) {
            error = "failed to decode thread/list result: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<typed::Thread> decodeThreadReadResult(const Json& value, std::string& error) {
        try {
            error.clear();

            if (!value.is_object()) {
                fail(error, "thread/read result must be an object");
                return std::nullopt;
            }

            const Json* thread = findMember(value, "thread");
            if (thread == nullptr) {
                fail(error, "thread/read result is missing required field 'thread'");
                return std::nullopt;
            }
            if (!thread->is_object()) {
                fail(error, "thread/read result field 'thread' must be an object");
                return std::nullopt;
            }

            return decodeThread(*thread, error);
        } catch (const std::exception& exception) {
            error = std::string("failed to decode thread/read result: ") + exception.what();
        } catch (...) {
            error = "failed to decode thread/read result: unknown local exception";
        }

        return std::nullopt;
    }

} // namespace ai::openai::codex::detail
