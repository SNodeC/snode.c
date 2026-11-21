// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/Server.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"

#include "Log.h"
#include "core/timer/Timer.h"

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <utility>

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

namespace mcp::server {
    using namespace detail;
    namespace {
        bool validId(const Json& id) {
            return id.is_string() || id.is_number_integer();
        }
        bool isType(const Json& value, const std::string& type) {
            const bool result = (type == "object" && value.is_object()) || (type == "array" && value.is_array()) ||
                                (type == "string" && value.is_string()) || (type == "number" && value.is_number()) ||
                                (type == "integer" && value.is_number() && std::isfinite(value.get<double>()) &&
                                 std::floor(value.get<double>()) >= value.get<double>()) ||
                                (type == "boolean" && value.is_boolean()) || (type == "null" && value.is_null());
            return result;
        }

        bool nonnegative(const Json& value) {
            return value.is_number_integer() && (value.is_number_unsigned() || value.get<std::int64_t>() >= 0);
        }

        bool schemaShape(const Json& schema, std::string& error, std::size_t depth, std::size_t& nodes) {
            static const std::set<std::string> keywords = {"$schema",
                                                           "type",
                                                           "properties",
                                                           "required",
                                                           "additionalProperties",
                                                           "items",
                                                           "enum",
                                                           "const",
                                                           "minimum",
                                                           "maximum",
                                                           "minLength",
                                                           "maxLength",
                                                           "minItems",
                                                           "maxItems",
                                                           "title",
                                                           "description",
                                                           "default"};
            static const std::set<std::string> types = {"object", "array", "string", "number", "integer", "boolean", "null"};
            ++nodes;
            bool good = depth <= 16 && nodes <= 256 && (schema.is_object() || schema.is_boolean());
            if (!good) {
                error = "schema shape/depth/node limit";
            } else if (schema.is_object()) {
                for (auto item = schema.begin(); item != schema.end() && good; ++item) {
                    const std::string& key = item.key();
                    const Json& value = item.value();
                    good = keywords.contains(key);
                    if (good && key == "$schema") {
                        good = value == "https://json-schema.org/draft/2020-12/schema" ||
                               value == "https://json-schema.org/draft/2020-12/schema#";
                    } else if (good && key == "type") {
                        Json alternatives = value.is_array() ? value : Json::array({value});
                        std::set<std::string> seen;
                        good = !alternatives.empty();
                        for (const auto& type : alternatives) {
                            good = good && type.is_string() && types.contains(type.get<std::string>()) &&
                                   seen.insert(type.get<std::string>()).second;
                        }
                    } else if (good && key == "properties") {
                        good = value.is_object();
                        for (auto property = value.begin(); property != value.end() && good; ++property) {
                            good = schemaShape(property.value(), error, depth + 1, nodes);
                        }
                    } else if (good && key == "items") {
                        good = schemaShape(value, error, depth + 1, nodes);
                    } else if (good && key == "required") {
                        good = value.is_array();
                        std::set<std::string> seen;
                        for (const auto& property : value) {
                            good = good && property.is_string() && seen.insert(property.get<std::string>()).second;
                        }
                    } else if (good && key == "additionalProperties") {
                        good = value.is_boolean();
                    } else if (good && (key == "minimum" || key == "maximum")) {
                        good = value.is_number() && std::isfinite(value.get<double>());
                    } else if (good && (key == "minLength" || key == "maxLength" || key == "minItems" || key == "maxItems")) {
                        good = nonnegative(value);
                    } else if (good && (key == "title" || key == "description")) {
                        good = value.is_string();
                    } else if (good && key == "enum") {
                        good = value.is_array() && !value.empty();
                        for (std::size_t i = 0; i < value.size() && good; ++i) {
                            for (std::size_t j = 0; j < i && good; ++j) {
                                good = value[i] != value[j];
                            }
                        }
                    }
                    if (!good && error.empty()) {
                        error = "unsupported or malformed schema keyword: " + key;
                    }
                }
            }
            return good;
        }

        bool matches(const Json& schema, const Json& value, std::string& error, std::size_t depth) {
            bool good = depth <= 64;
            if (schema.is_boolean()) {
                good = good && schema.get<bool>();
            } else if (good) {
                if (schema.contains("type")) {
                    const Json alternatives = schema["type"].is_array() ? schema["type"] : Json::array({schema["type"]});
                    good = std::any_of(alternatives.begin(), alternatives.end(), [&value](const Json& type) {
                        return isType(value, type.get<std::string>());
                    });
                }
                if (schema.contains("enum")) {
                    good = good && std::find(schema["enum"].begin(), schema["enum"].end(), value) != schema["enum"].end();
                }
                if (schema.contains("const")) {
                    good = good && value == schema["const"];
                }
                if (value.is_number()) {
                    good = good && (!schema.contains("minimum") || value >= schema["minimum"]) &&
                           (!schema.contains("maximum") || value <= schema["maximum"]);
                }
                if (value.is_string()) {
                    const std::string& text = value.get_ref<const std::string&>();
                    const auto length = static_cast<std::size_t>(std::count_if(text.begin(), text.end(), [](char c) {
                        return (static_cast<unsigned char>(c) & 0xc0U) != 0x80U;
                    }));
                    good = good && (!schema.contains("minLength") || Json(length) >= schema["minLength"]) &&
                           (!schema.contains("maxLength") || Json(length) <= schema["maxLength"]);
                }
                if (value.is_array()) {
                    good = good && (!schema.contains("minItems") || Json(value.size()) >= schema["minItems"]) &&
                           (!schema.contains("maxItems") || Json(value.size()) <= schema["maxItems"]);
                    for (auto item = value.begin(); item != value.end() && good && schema.contains("items"); ++item) {
                        good = matches(schema["items"], *item, error, depth + 1);
                    }
                }
                if (value.is_object()) {
                    for (const auto& required : schema.value("required", Json::array())) {
                        good = good && value.contains(required.get<std::string>());
                    }
                    const Json properties = schema.value("properties", Json::object());
                    for (auto item = value.begin(); item != value.end() && good; ++item) {
                        if (properties.contains(item.key())) {
                            good = matches(properties[item.key()], item.value(), error, depth + 1);
                        } else {
                            good = schema.value("additionalProperties", true);
                        }
                    }
                }
            }
            if (!good && error.empty()) {
                error = "arguments do not satisfy the registered schema";
            }
            return good;
        }
    } // namespace

    detail::Response::Response(Json id, bool modern, bool cacheable, Output output, const utils::Timeval& timeout)
        : id(validId(id) ? std::move(id) : Json())
        , modern(modern)
        , cacheable(cacheable)
        , output(std::move(output))
        , deadline(utils::Timeval::currentTime() + timeout) {
    }
    detail::Response::~Response() {
        timer.cancel();
    }
    bool detail::Response::active() const {
        return !done;
    }
    bool detail::Response::expired() const {
        return utils::Timeval::currentTime() >= deadline;
    }
    void detail::Response::expire() {
        if (active() && expired() && !subscription) {
            cancel(CancelReason::Deadline);
        }
    }
    bool detail::Response::isSubscription() const {
        return subscription;
    }
    void detail::Response::result(Json value) {
        const auto keep = weak_from_this().lock();
        if (!cancellation) {
            expire();
        }
        if (!done) {
            std::string validationError;
            if (!outputSchema.is_null() && value.value("isError", Json()) != true &&
                (!value.contains("structuredContent") ||
                 !Access::Registry::validate(outputSchema, value["structuredContent"], validationError))) {
                value = {{"isError", true}, {"content", {{{"type", "text"}, {"text", "Tool output failed its registered schema"}}}}};
            }
            if (!modern && value.contains("structuredContent") && !value["structuredContent"].is_object()) {
                value.erase("structuredContent");
            }
            if (modern) {
                value["resultType"] = "complete";
                if (cacheable) {
                    value["ttlMs"] = 0;
                    value["cacheScope"] = "private";
                }
            }
            done = true;
            timer.cancel();
            observers.clear();
            output(Json{{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(value)}}, true);
        }
    }
    void detail::Response::error(int code, const std::string& message, Json data) {
        const auto keep = weak_from_this().lock();
        if (!cancellation) {
            expire();
        }
        if (!done) {
            Json response{{"jsonrpc", "2.0"}, {"error", {{"code", code}, {"message", message}}}};
            if (!id.is_null() || !modern) {
                response["id"] = id;
            }
            if (!data.is_null()) {
                response["error"]["data"] = std::move(data);
            }
            done = true;
            timer.cancel();
            observers.clear();
            output(response, true);
        }
    }
    void detail::Response::notify(const std::string& method, Json params) {
        if (!done) {
            if (subscription) {
                params["_meta"][subscriptionKey] = id;
            }
            output(Json{{"jsonrpc", "2.0"}, {"method", method}, {"params", std::move(params)}}, false);
        }
    }
    void detail::Response::listen() {
        subscription = true;
        notify("notifications/subscriptions/acknowledged", {{"notifications", Json::object()}});
    }
    void detail::Response::observe(RequestContext::OnCancel callback) {
        if (callback) {
            if (cancellation) {
                try {
                    callback(*cancellation);
                } catch (const std::exception& error) {
                    snode::log::framework("mcp.server", snode::log::Boundary::Application).error() << "cancel observer: " << error.what();
                } catch (...) {
                    snode::log::framework("mcp.server", snode::log::Boundary::Application).error() << "cancel observer failed";
                }
            } else if (active()) {
                observers.push_back(std::move(callback));
            }
        }
    }
    void detail::Response::cancel(CancelReason reason) {
        const auto keep = weak_from_this().lock();
        if (!done) {
            cancellation = reason;
            auto callbacks = std::move(observers);
            if (reason == CancelReason::Deadline) {
                if (toolRequest) {
                    result({{"isError", true}, {"content", {{{"type", "text"}, {"text", "Request deadline exceeded"}}}}});
                } else {
                    error(-32603, "Request deadline exceeded");
                }
            } else {
                if (reason == CancelReason::Shutdown && subscription) {
                    notify("notifications/cancelled", {{"requestId", id}, {"reason", "subscription closed"}});
                }
                done = true;
                timer.cancel();
                output(nullptr, true);
            }
            for (auto& callback : callbacks) {
                observe(std::move(callback));
            }
        }
    }
    Server::Impl::Impl(std::string name, std::string version, std::string instructions)
        : name(std::move(name))
        , version(std::move(version))
        , instructions(std::move(instructions)) {
    }
    bool Server::Impl::checkSchema(const Json& schema, std::string& error) {
        std::size_t nodes = 0;
        error.clear();
        return schema.dump().size() <= 16384 && schemaShape(schema, error, 0, nodes);
    }
    bool Server::Impl::validate(const Json& schema, const Json& value, std::string& error) {
        error.clear();
        return matches(schema, value, error, 0);
    }
    void Server::Impl::add(RegistrationKind kind, Json description, Handler handler) {
        const std::string keyName = kind == Kind::Resource ? "uri" : "name";
        const std::string identifier = description.value(keyName, std::string());
        const std::string key = std::to_string(static_cast<int>(kind)) + ":" + identifier;
        const auto reject = [&](const std::string& field, const std::string& reason) {
            throw RegistrationError(kind, identifier, field, reason);
        };
        if (sealed) {
            reject("registration", "registry is sealed");
        } else if (identifier.empty() || description.value("name", std::string()).empty()) {
            reject(identifier.empty() ? keyName : "name", "must not be empty");
        } else if (!std::visit(
                       [](const auto& callback) {
                           return static_cast<bool>(callback);
                       },
                       handler)) {
            reject("handler", "must be callable");
        } else if (entries.size() >= 128 || entries.contains(key)) {
            reject(keyName, entries.contains(key) ? "duplicate registration" : "128-registration limit exceeded");
        }
        if (kind == Kind::Tool) {
            std::string error;
            if (description.contains("annotations") && !description["annotations"].is_object()) {
                reject("annotations", "must be an object");
            }
            const Json& input = description["inputSchema"];
            if (!input.is_object() || input.value("type", Json()) != "object" || !checkSchema(input, error)) {
                reject("inputSchema", error.empty() ? "supported object schema required" : error);
            }
            if (description.contains("outputSchema") && !checkSchema(description["outputSchema"], error)) {
                reject("outputSchema", error.empty() ? "supported schema required" : error);
            }
        } else if (kind == Kind::Prompt) {
            std::set<std::string> names;
            for (const auto& argument : description["arguments"]) {
                const std::string name = argument["name"];
                if (name.empty() || !names.insert(name).second) {
                    reject("arguments.name", "must be nonempty and unique");
                }
            }
        }
        entries.emplace(key, Entry{kind, std::move(description), std::move(handler)});
    }
    void Server::Impl::seal() {
        sealed = true;
    }
    bool Server::Impl::validRequest(const Json& request) {
        const Json method = request.is_object() ? request.value("method", Json()) : Json();
        return request.is_object() && request.value("jsonrpc", Json()) == "2.0" && method.is_string() && method != "" &&
               (!request.contains("id") || validId(request["id"])) && (!request.contains("params") || request["params"].is_object());
    }
    Json Server::Impl::capabilities() const {
        Json result = Json::object();
        for (const auto& [key, entry] : entries) {
            static_cast<void>(key);
            result[entry.kind == Kind::Tool ? "tools" : entry.kind == Kind::Resource ? "resources" : "prompts"] = Json::object();
        }
        return result;
    }
    std::shared_ptr<detail::Access::Response>
    Server::Impl::dispatch(const Json& request, bool legacy, detail::Access::Response::Output output, const utils::Timeval& timeout) const {
        const Json id = request.is_object() ? request.value("id", Json()) : Json();
        const std::string method = request.is_object() && request.contains("method") && request["method"].is_string()
                                       ? request["method"].get<std::string>()
                                       : std::string();
        auto reply = std::make_shared<detail::Access::Response>(id,
                                                                !legacy,
                                                                method == "server/discover" || method.ends_with("/list") ||
                                                                    method == "resources/read",
                                                                std::move(output),
                                                                timeout);
        reply->toolRequest = method == "tools/call";
        if (legacy || method != "subscriptions/listen") {
            const std::weak_ptr<Response> weak = reply;
            reply->timer = core::timer::Timer::singleshotTimer(
                [weak] {
                    if (const auto current = weak.lock()) {
                        current->cancel(CancelReason::Deadline);
                    }
                },
                timeout);
        }
        try {
            if (!validRequest(request)) {
                reply->error(-32600, "Invalid Request");
            } else if (!request.contains("id")) {
                reply->cancel();
            } else {
                const Json params = request.value("params", Json::object());
                const Json meta = params.value("_meta", Json::object());
                const Json version = meta.is_object() ? meta.value(versionKey, Json()) : Json();
                const Json client = params.value("clientInfo", Json());
                const Json token = meta.is_object() ? meta.value("progressToken", Json()) : Json();
                const bool validToken =
                    !meta.is_object() || !meta.contains("progressToken") || token.is_string() ||
                    (token.is_number_integer() && (!token.is_number_unsigned() || token.get<std::uint64_t>() <= INT64_MAX));
                if (token.is_string()) {
                    reply->progressToken = token.get<std::string>();
                } else if (validToken && !token.is_null()) {
                    reply->progressToken = token.get<std::int64_t>();
                }
                if (!legacy &&
                    (!meta.is_object() || !version.is_string() || !meta.contains(capabilitiesKey) || !meta[capabilitiesKey].is_object())) {
                    reply->error(-32602, "Missing or malformed per-request metadata");
                } else if (!validToken) {
                    reply->error(-32602, "Progress token must be a string or signed integer");
                } else if (!legacy && version != modernVersion) {
                    reply->error(
                        -32022, "Unsupported protocol version", {{"supported", {modernVersion, legacyVersion}}, {"requested", version}});

                } else if (legacy && method == "initialize" &&
                           (!params.value("protocolVersion", Json()).is_string() || !params.value("capabilities", Json()).is_object() ||
                            !client.is_object() || !client.value("name", Json()).is_string() ||
                            !client.value("version", Json()).is_string())) {
                    reply->error(-32602, "Malformed initialize parameters");
                } else if ((!legacy && method == "server/discover") || (legacy && method == "initialize")) {
                    Json value{{"capabilities", capabilities()}};
                    if (legacy) {
                        value["protocolVersion"] = legacyVersion;
                        value["serverInfo"] = {{"name", name}, {"version", this->version}};
                    } else {
                        value["supportedVersions"] = {modernVersion, legacyVersion};
                        value["_meta"]["io.modelcontextprotocol/serverInfo"] = {{"name", name}, {"version", this->version}};
                    }
                    if (!instructions.empty()) {
                        value["instructions"] = instructions;
                    }
                    reply->result(std::move(value));
                } else if (legacy && method == "ping") {
                    reply->result(Json::object());
                } else if (!legacy && method == "subscriptions/listen") {
                    if (params.contains("notifications") && !params["notifications"].is_object()) {
                        reply->error(-32602, "Invalid subscription filter");
                    } else {
                        reply->listen();
                    }
                } else if (method == "resources/templates/list") {
                    reply->result({{"resourceTemplates", Json::array()}});
                } else if (method == "tools/list" || method == "resources/list" || method == "prompts/list") {
                    const Kind kind = method == "tools/list" ? Kind::Tool : method == "resources/list" ? Kind::Resource : Kind::Prompt;
                    Json list = Json::array();
                    for (const auto& [key, entry] : entries) {
                        static_cast<void>(key);
                        if (entry.kind == kind) {
                            Json description = entry.description;
                            if (legacy && kind == Kind::Tool && description.contains("outputSchema") &&
                                (!description["outputSchema"].is_object() ||
                                 description["outputSchema"].value("type", Json()) != "object")) {
                                description.erase("outputSchema");
                            }
                            list.push_back(std::move(description));
                        }
                    }
                    reply->result({{method.substr(0, method.find('/')), std::move(list)}});
                } else if (method == "tools/call" || method == "resources/read" || method == "prompts/get") {
                    const Kind kind = method == "tools/call" ? Kind::Tool : method == "resources/read" ? Kind::Resource : Kind::Prompt;
                    const std::string argument = kind == Kind::Resource ? "uri" : "name";
                    if (!params.contains(argument) || !params[argument].is_string()) {
                        reply->error(-32602, "Missing name or uri");
                    } else {
                        const auto entry = entries.find(std::to_string(static_cast<int>(kind)) + ":" + params[argument].get<std::string>());
                        const Json arguments = params.value("arguments", Json::object());
                        std::string error;
                        if (entry == entries.end() || !arguments.is_object() ||
                            (kind == Kind::Prompt && !std::all_of(arguments.begin(), arguments.end(), [](const Json& value) {
                                 return value.is_string();
                             }))) {
                            reply->error(legacy && kind == Kind::Resource && entry == entries.end() ? -32002 : -32602,
                                         "Unknown registration or malformed arguments");
                        } else if (kind == Kind::Tool && !validate(entry->second.description["inputSchema"], arguments, error)) {
                            reply->result({{"isError", true}, {"content", {{{"type", "text"}, {"text", error}}}}});
                        } else {
                            if (kind == Kind::Prompt) {
                                for (const auto& argument : entry->second.description["arguments"]) {
                                    if (argument.value("required", false) && !arguments.contains(argument["name"].get<std::string>())) {
                                        reply->error(-32602, "Missing required prompt argument: " + argument["name"].get<std::string>());
                                    }
                                }
                            }
                            if (entry->second.description.contains("outputSchema")) {
                                reply->outputSchema = entry->second.description["outputSchema"];
                            }
                            if (kind == Kind::Tool) {
                                std::get<Server::ToolHandler>(entry->second.handler)(ToolCall{Access::context(reply), arguments},
                                                                                     Access::wrap<ToolResult>(reply));
                            } else if (kind == Kind::Resource) {
                                std::get<Server::ResourceHandler>(entry->second.handler)(
                                    ResourceRead{Access::context(reply), params["uri"]}, Access::wrap<ResourceResult>(reply));
                            } else if (reply->active()) {
                                std::get<Server::PromptHandler>(entry->second.handler)(
                                    PromptGet{Access::context(reply), arguments.get<std::map<std::string, std::string>>()},
                                    Access::wrap<PromptResult>(reply));
                            }
                        }
                    }
                } else {
                    reply->error(-32601, "Method not found");
                }
            }
        } catch (const std::exception& error) {
            snode::log::framework("mcp.server", snode::log::Boundary::Application).error() << "dispatch failed: " << error.what();
            reply->error(-32603, "Internal error");
        }
        reply->expire();
        return reply;
    }
    int Server::Impl::httpStatus(const Json& message) {
        int status = 200;
        if (message.contains("error")) {
            const int code = message["error"]["code"].get<int>();
            status = code == -32601 ? 404 : code == -32603 ? 500 : 400;
        }
        return status;
    }

    namespace {
        Json encode(const ResourceContents& contents) {
            return std::visit(
                [](const auto& resource) {
                    Json value{{"uri", resource.uri}, {"mimeType", resource.mimeType}};
                    if constexpr (std::is_same_v<std::decay_t<decltype(resource)>, TextResource>) {
                        value["text"] = resource.text;
                    } else {
                        value["blob"] = resource.base64;
                    }
                    return value;
                },
                contents);
        }
        Json encode(const Content& content) {
            return std::visit(
                [](const auto& block) {
                    using Block = std::decay_t<decltype(block)>;
                    Json value;
                    if constexpr (std::is_same_v<Block, TextContent>) {
                        value = {{"type", "text"}, {"text", block.text}};
                    } else if constexpr (std::is_same_v<Block, EmbeddedResource>) {
                        value = {{"type", "resource"}, {"resource", encode(block.resource)}};
                    } else if constexpr (std::is_same_v<Block, ResourceLink>) {
                        value = {{"type", "resource_link"},
                                 {"uri", block.uri},
                                 {"name", block.name},
                                 {"description", block.description},
                                 {"mimeType", block.mimeType}};
                        if (block.size) {
                            value["size"] = *block.size;
                        }
                    } else {
                        value = {{"type", std::is_same_v<Block, ImageContent> ? "image" : "audio"},
                                 {"data", block.base64},
                                 {"mimeType", block.mimeType}};
                    }
                    const auto& annotations = block.annotations;
                    if (!annotations.audience.empty()) {
                        value["annotations"]["audience"] = Json::array();
                        for (Role role : annotations.audience) {
                            value["annotations"]["audience"].push_back(role == Role::User ? "user" : "assistant");
                        }
                    }
                    if (annotations.priority) {
                        value["annotations"]["priority"] = *annotations.priority;
                    }
                    if (annotations.lastModified) {
                        value["annotations"]["lastModified"] = *annotations.lastModified;
                    }
                    return value;
                },
                content);
        }
    } // namespace
    ToolResult::ToolResult(Json value)
        : value(std::move(value)) {
    }
    ToolResult ToolResult::text(std::string text) {
        return content({TextContent{std::move(text), {}}});
    }
    ToolResult ToolResult::json(Json value) {
        ToolResult result = text(value.dump());
        result.value["structuredContent"] = std::move(value);
        return result;
    }
    ToolResult ToolResult::content(std::vector<Content> content) {
        Json blocks = Json::array();
        for (const auto& block : content) {
            blocks.push_back(encode(block));
        }
        return ToolResult({{"content", std::move(blocks)}});
    }
    ToolResult ToolResult::toolError(std::string message) {
        return toolError({TextContent{std::move(message), {}}});
    }
    ToolResult ToolResult::toolError(std::vector<Content> content) {
        ToolResult result = ToolResult::content(std::move(content));
        result.value["isError"] = true;
        return result;
    }
    template <class Result>
    Reply<Result>::Reply(std::shared_ptr<detail::Response> impl)
        : impl(std::move(impl)) {
    }
    template <class Result>
    Reply<Result>::~Reply() = default;
    template <class Result>
    Completion Reply<Result>::result(Result result) {
        const auto keep = impl;
        impl->expire();
        const Completion completion = impl->active() ? Completion::Accepted : Completion::Inactive;
        if constexpr (std::is_same_v<Result, ToolResult>) {
            impl->result(std::move(result.value));
        } else if constexpr (std::is_same_v<Result, ResourceResult>) {
            Json contents = Json::array();
            for (const auto& resource : result.contents) {
                contents.push_back(encode(resource));
            }
            impl->result({{"contents", std::move(contents)}});
        } else {
            Json value{{"messages", Json::array()}};
            if (result.description) {
                value["description"] = *result.description;
            }
            for (const auto& message : result.messages) {
                value["messages"].push_back(
                    {{"role", message.role == Role::User ? "user" : "assistant"}, {"content", encode(message.content)}});
            }
            impl->result(std::move(value));
        }
        return completion;
    }
    template <class Result>
    Completion Reply<Result>::error(ErrorCode code, std::string message, Json data) {
        const auto keep = impl;
        impl->expire();
        const Completion completion = impl->active() ? Completion::Accepted : Completion::Inactive;
        impl->error(static_cast<int>(code), message, std::move(data));
        return completion;
    }
    template <class Result>
    void Reply<Result>::progress(Progress progress) {
        impl->expire();
        if (impl->progressToken && impl->active() && std::isfinite(progress.value) && progress.value >= 0 &&
            (!impl->lastProgress || progress.value >= *impl->lastProgress) &&
            (!progress.total || (std::isfinite(*progress.total) && *progress.total >= progress.value))) {
            impl->lastProgress = progress.value;
            Json params{{"progress", progress.value}};
            params["progressToken"] = std::visit(
                [](const auto& token) {
                    return Json(token);
                },
                *impl->progressToken);
            if (progress.total) {
                params["total"] = *progress.total;
            }
            if (!progress.message.empty()) {
                params["message"] = std::move(progress.message);
            }
            impl->notify("notifications/progress", std::move(params));
        }
    }
    RequestContext::RequestContext(std::shared_ptr<detail::Response> impl)
        : impl(std::move(impl)) {
    }
    const std::optional<Principal>& RequestContext::getPrincipal() const {
        return impl->principal;
    }
    const std::optional<ProgressToken>& RequestContext::getProgressToken() const {
        return impl->progressToken;
    }
    void RequestContext::onCancel(OnCancel callback) const {
        impl->observe(std::move(callback));
    }
    template class Reply<ToolResult>;
    template class Reply<ResourceResult>;
    template class Reply<PromptResult>;

    RegistrationError::RegistrationError(RegistrationKind kind, std::string identifier, std::string field, std::string reason)
        : std::invalid_argument(std::string(kind == RegistrationKind::Tool       ? "tool "
                                            : kind == RegistrationKind::Resource ? "resource "
                                                                                 : "prompt ") +
                                (identifier.empty() ? "<unnamed>" : identifier) + ": " + field + ": registration rejected: " + reason) {
    }
    Server::Server(std::string name, std::string version, std::string instructions)
        : impl(std::make_unique<Impl>(std::move(name), std::move(version), std::move(instructions))) {
    }
    Server::~Server() = default;
    Server* Server::tool(ToolDefinition definition, ToolHandler handler) {
        Json description{{"name", definition.name}, {"description", definition.description}, {"inputSchema", definition.inputSchema}};
        if (definition.outputSchema) {
            description["outputSchema"] = *definition.outputSchema;
        }
        if (definition.title) {
            description["title"] = *definition.title;
        }
        if (!definition.annotations.is_null()) {
            description["annotations"] = std::move(definition.annotations);
        }
        impl->add(RegistrationKind::Tool, std::move(description), std::move(handler));
        return this;
    }
    Server* Server::resource(ResourceDefinition definition, ResourceHandler handler) {
        Json description{
            {"uri", definition.uri}, {"name", definition.name}, {"description", definition.description}, {"mimeType", definition.mimeType}};
        if (definition.title) {
            description["title"] = *definition.title;
        }
        impl->add(RegistrationKind::Resource, std::move(description), std::move(handler));
        return this;
    }
    Server* Server::prompt(PromptDefinition definition, PromptHandler handler) {
        Json description{{"name", definition.name}, {"description", definition.description}, {"arguments", Json::array()}};
        for (const auto& argument : definition.arguments) {
            description["arguments"].push_back({{"name", argument.name},
                                                {"description", argument.description},
                                                {"required", argument.requirement == Requirement::Required}});
        }
        if (definition.title) {
            description["title"] = *definition.title;
        }
        impl->add(RegistrationKind::Prompt, std::move(description), std::move(handler));
        return this;
    }
    detail::Access::Registry& detail::Access::registry(const Server& server) {
        return *server.impl;
    }
} // namespace mcp::server
