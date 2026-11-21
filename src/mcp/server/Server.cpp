// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/Server.h"

#include "Log.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

namespace mcp::server {
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

    Reply::Reply(Json id, bool modern, bool cacheable, Output output, std::chrono::milliseconds timeout)
        : id(validId(id) ? std::move(id) : Json())
        , modern(modern)
        , cacheable(cacheable)
        , output(std::move(output))
        , deadline(std::chrono::steady_clock::now() + timeout) {
    }
    bool Reply::active() const {
        return !done;
    }
    bool Reply::expired() const {
        return std::chrono::steady_clock::now() >= deadline;
    }
    bool Reply::isSubscription() const {
        return subscription;
    }
    void Reply::result(Json value) {
        if (!done) {
            std::string validationError;
            if (!outputSchema.is_null() && value.value("isError", Json()) != true &&
                (!value.contains("structuredContent") || !Server::validate(outputSchema, value["structuredContent"], validationError))) {
                value = {{"isError", true}, {"content", {{{"type", "text"}, {"text", "Tool output failed its registered schema"}}}}};
            }
            if (modern) {
                value["resultType"] = "complete";
                if (cacheable) {
                    value["ttlMs"] = 0;
                    value["cacheScope"] = "private";
                }
            }
            done = true;
            output(Json{{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(value)}}, true);
        }
    }
    void Reply::error(int code, const std::string& message, Json data) {
        if (!done) {
            Json response{{"jsonrpc", "2.0"}, {"error", {{"code", code}, {"message", message}}}};
            if (!id.is_null() || !modern) {
                response["id"] = id;
            }
            if (!data.is_null()) {
                response["error"]["data"] = std::move(data);
            }
            done = true;
            output(response, true);
        }
    }
    void Reply::notify(const std::string& method, Json params) {
        if (!done) {
            if (subscription) {
                params["_meta"][subscriptionKey] = id;
            }
            output(Json{{"jsonrpc", "2.0"}, {"method", method}, {"params", std::move(params)}}, false);
        }
    }
    void Reply::listen() {
        subscription = true;
        notify("notifications/subscriptions/acknowledged", {{"notifications", Json::object()}});
    }
    void Reply::cancel(bool serverInitiated) {
        if (!done) {
            if (serverInitiated && subscription) {
                notify("notifications/cancelled", {{"requestId", id}, {"reason", "subscription closed"}});
            }
            done = true;
            output(nullptr, true);
        }
    }
    Server::Server(std::string name)
        : name(std::move(name)) {
    }
    bool Server::checkSchema(const Json& schema, std::string& error) {
        std::size_t nodes = 0;
        error.clear();
        return schema.dump().size() <= 16384 && schemaShape(schema, error, 0, nodes);
    }
    bool Server::validate(const Json& schema, const Json& value, std::string& error) {
        error.clear();
        return matches(schema, value, error, 0);
    }
    void Server::add(Kind kind, Json description, Handler handler) {
        const std::string keyName = kind == Kind::Resource ? "uri" : "name";
        std::string error;
        bool good = description.is_object() && description.contains(keyName) && description[keyName].is_string() &&
                    !description[keyName].get_ref<const std::string&>().empty() && static_cast<bool>(handler) && entries.size() < 128;
        if (good && kind == Kind::Tool) {
            good = description.contains("inputSchema") && description["inputSchema"].is_object() &&
                   description["inputSchema"].value("type", Json()) == "object" && checkSchema(description["inputSchema"], error);
            if (good && description.contains("outputSchema")) {
                good = checkSchema(description["outputSchema"], error);
            }
        }
        if (good) {
            const std::string key = std::to_string(static_cast<int>(kind)) + ":" + description[keyName].get<std::string>();
            good = entries.emplace(key, Entry{kind, std::move(description), std::move(handler)}).second;
        }
        if (!good) {
            registrationsValid = false;
            snode::log::framework("mcp.server", snode::log::Boundary::Configuration).error() << "registration rejected: " << error;
        }
    }
    void Server::tool(Json description, Handler handler) {
        add(Kind::Tool, std::move(description), std::move(handler));
    }
    void Server::resource(Json description, Handler handler) {
        add(Kind::Resource, std::move(description), std::move(handler));
    }
    void Server::prompt(Json description, Handler handler) {
        add(Kind::Prompt, std::move(description), std::move(handler));
    }
    bool Server::valid() const {
        return registrationsValid;
    }
    bool Server::validRequest(const Json& request) {
        const Json method = request.is_object() ? request.value("method", Json()) : Json();
        return request.is_object() && request.value("jsonrpc", Json()) == "2.0" && method.is_string() && method != "" &&
               (!request.contains("id") || validId(request["id"])) && (!request.contains("params") || request["params"].is_object());
    }
    Json Server::capabilities() const {
        Json result = Json::object();
        for (const auto& [key, entry] : entries) {
            static_cast<void>(key);
            result[entry.kind == Kind::Tool ? "tools" : entry.kind == Kind::Resource ? "resources" : "prompts"] = Json::object();
        }
        return result;
    }
    std::shared_ptr<Reply>
    Server::dispatch(const Json& request, bool legacy, Reply::Output output, std::chrono::milliseconds timeout) const {
        const Json id = request.is_object() ? request.value("id", Json()) : Json();
        const std::string method = request.is_object() && request.contains("method") && request["method"].is_string()
                                       ? request["method"].get<std::string>()
                                       : std::string();
        auto reply = std::make_shared<Reply>(id,
                                             !legacy,
                                             method == "server/discover" || method.ends_with("/list") || method == "resources/read",
                                             std::move(output),
                                             timeout);
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
                if (!legacy &&
                    (!meta.is_object() || !version.is_string() || !meta.contains(capabilitiesKey) || !meta[capabilitiesKey].is_object())) {
                    reply->error(-32602, "Missing or malformed per-request metadata");
                } else if (!legacy && version != modernVersion) {
                    reply->error(
                        -32022, "Unsupported protocol version", {{"supported", {modernVersion, legacyVersion}}, {"requested", version}});
                } else if (!valid()) {
                    reply->error(-32603, "Invalid server registration");
                } else if (legacy && method == "initialize" &&
                           (!params.value("protocolVersion", Json()).is_string() || !params.value("capabilities", Json()).is_object() ||
                            !client.is_object() || !client.value("name", Json()).is_string() ||
                            !client.value("version", Json()).is_string())) {
                    reply->error(-32602, "Malformed initialize parameters");
                } else if ((!legacy && method == "server/discover") || (legacy && method == "initialize")) {
                    Json value{{"capabilities", capabilities()}};
                    if (legacy) {
                        value["protocolVersion"] = legacyVersion;
                        value["serverInfo"] = {{"name", name}, {"version", "1"}};
                    } else {
                        value["supportedVersions"] = {modernVersion, legacyVersion};
                        value["_meta"]["io.modelcontextprotocol/serverInfo"] = {{"name", name}, {"version", "1"}};
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
                            list.push_back(entry.description);
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
                        if (entry == entries.end() || !arguments.is_object()) {
                            reply->error(legacy && kind == Kind::Resource && entry == entries.end() ? -32002 : -32602,
                                         "Unknown registration or malformed arguments");
                        } else if (kind == Kind::Tool && !validate(entry->second.description["inputSchema"], arguments, error)) {
                            reply->result({{"isError", true}, {"content", {{{"type", "text"}, {"text", error}}}}});
                        } else {
                            if (entry->second.description.contains("outputSchema")) {
                                reply->outputSchema = entry->second.description["outputSchema"];
                            }
                            entry->second.handler(kind == Kind::Resource ? params : arguments, reply);
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
        return reply;
    }
    int Server::httpStatus(const Json& message) {
        int status = 200;
        if (message.contains("error")) {
            const int code = message["error"]["code"].get<int>();
            status = code == -32601 ? 404 : code == -32603 ? 500 : 400;
        }
        return status;
    }
} // namespace mcp::server
