/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "utils/ConfigJsonFormatter.h"

#include "utils/JsonWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    namespace {

        struct Classification {
            std::string kind;
            std::string source;
        };

        struct OptionValueState {
            std::string apiDefault;
            std::string configured;
            std::string effective;
            std::string source;
            bool isConfigured = false;
            bool isDefault = false;
            bool isMissingRequired = false;
        };

        std::string lowerCopy(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            return value;
        }

        std::string displayNameForKey(const std::string& key) {
            std::string displayName = key;
            const std::string::size_type pos = key.find_last_of('.');

            if (pos != std::string::npos) {
                displayName = key.substr(pos + 1);
            }

            return displayName;
        }

        Classification classifyOptionGroup(const std::string& group) {
            Classification classification{"custom", "heuristic-group-name"};
            const std::string lower = lowerCopy(group);

            if (group.empty() || group == "Options") {
                classification.kind = "default";
                classification.source = "cli11-default-group";
            } else if (lower.find("nonpersistent") != std::string::npos) {
                classification.kind = "nonpersistent";
            } else if (lower.find("persistent") != std::string::npos) {
                classification.kind = "persistent";
            }

            return classification;
        }

        Classification classifyNode(const CLI::App* app, bool root) {
            Classification classification{"subcommand", "fallback"};

            if (root) {
                classification.kind = "application";
                classification.source = "root";
            } else if (app->get_name().empty()) {
                classification.kind = "anonymous";
                classification.source = "cli11-name";
            } else if (app->get_group() == "Instances") {
                classification.kind = "instance";
                classification.source = "cli11-group";
            } else if (app->get_group() == "Sections") {
                classification.kind = "section";
                classification.source = "cli11-group";
            } else if (!app->get_group().empty()) {
                classification.kind = "category";
                classification.source = "heuristic-cli11-group";
            }

            return classification;
        }

        Classification classifyType(const CLI::Option* opt, const std::string& key) {
            Classification classification{"string", "fallback"};
            const std::string combined = lowerCopy(opt->get_type_name() + " " + key);

            if (opt->get_items_expected_max() == 0 || combined.find("bool") != std::string::npos) {
                classification.kind = "boolean";
                classification.source = "cli11-items-or-type-name";
            } else if (combined.find('{') != std::string::npos) {
                classification.kind = "enum";
                classification.source = "cli11-type-name";
            } else if (combined.find("port") != std::string::npos || combined.find("int") != std::string::npos ||
                       combined.find("level") != std::string::npos) {
                classification.kind = "integer";
                classification.source = "heuristic-name";
            } else if (combined.find("file") != std::string::npos || combined.find("path") != std::string::npos ||
                       combined.find("config") != std::string::npos) {
                classification.kind = "path";
                classification.source = "heuristic-name";
            }

            return classification;
        }

        std::string optionSingleName(const CLI::Option* opt) {
            std::string name = opt->get_single_name();

            if (name.rfind("--", 0) == 0) {
                name.erase(0, 2);
            } else if (name.rfind('-', 0) == 0) {
                name.erase(0, 1);
            }

            return name;
        }

        std::string configuredValue(const CLI::Option* opt) {
            std::string value;

            try {
                value = CLI::detail::ini_join(opt->reduced_results(), ' ', '[', ']', '"', '\'');
                if (value.empty() && !opt->results().empty()) {
                    value = CLI::detail::ini_join(opt->results(), ' ', '[', ']', '"', '\'');
                }
            } catch (const CLI::ParseError& e) {
                value = std::string{"<"} + e.get_name() + ": " + e.what() + ">";
            }

            return value;
        }

        std::string apiDefaultValue(const CLI::Option* opt) {
            std::string value = opt->get_default_str();

            if (value.empty()) {
                if (opt->get_required()) {
                    value = "<REQUIRED>";
                } else if (opt->get_expected_min() == 0) {
                    value = "false";
                }
            }

            return value;
        }

        OptionValueState extractValueState(const CLI::Option* opt) {
            OptionValueState state;

            state.configured = configuredValue(opt);
            state.isConfigured = opt->count() > 0 && !state.configured.empty();
            state.apiDefault = apiDefaultValue(opt);
            state.effective = state.isConfigured ? state.configured : state.apiDefault;
            state.isMissingRequired = opt->get_required() && (state.effective.empty() || state.effective == "<REQUIRED>");
            state.isDefault = !state.isConfigured && state.effective == state.apiDefault;

            if (state.isConfigured) {
                state.source = "command-line-or-config";
            } else if (state.isMissingRequired) {
                state.source = "required-placeholder";
            } else if (state.apiDefault.empty()) {
                state.source = "empty-default";
            } else {
                state.source = "api-default";
            }

            return state;
        }

        std::string valueItemsKind(const CLI::Option* opt) {
            std::string items = "single";

            if (opt->get_items_expected_max() > 1) {
                items = "list";
            }

            return items;
        }

        void writeStringArray(JsonWriter& json, const std::vector<std::string>& values) {
            json.beginArray();
            for (const std::string& value : values) {
                json.string(value);
            }
            json.endArray();
        }

        void writeCommandLine(JsonWriter& json, const CLI::Option* opt, const std::string& key) {
            json.key("commandLine");
            json.beginObject();

            json.key("long");
            if (!opt->get_lnames().empty()) {
                json.string("--" + opt->get_lnames().front());
            } else {
                json.string("--" + key);
            }

            json.key("short");
            if (!opt->get_snames().empty()) {
                json.string("-" + opt->get_snames().front());
            } else {
                json.null();
            }

            json.key("takesValue");
            json.boolean(opt->get_items_expected_max() != 0);

            json.key("valueSeparator");
            json.string(" ");

            json.key("repeatable");
            json.boolean(opt->get_expected_max() > 1);

            json.endObject();
        }

        void writeConfigFile(JsonWriter& json, const CLI::Option* opt, const std::string& key) {
            json.key("configFile");
            json.beginObject();

            json.key("key");
            json.string(key);

            json.key("section");
            json.null();

            json.key("writable");
            json.boolean(opt->get_configurable());

            json.key("writableSource");
            json.string("cli11-configurable");

            json.endObject();
        }

        void writeType(JsonWriter& json, const CLI::Option* opt, const std::string& key) {
            const Classification type = classifyType(opt, key);

            json.key("type");
            json.beginObject();

            json.key("kind");
            json.string(type.kind);

            json.key("kindSource");
            json.string(type.source);

            json.key("name");
            json.string(opt->get_type_name());

            json.key("cpp");
            json.null();

            json.key("items");
            json.string(valueItemsKind(opt));

            json.endObject();
        }

        void writeConstraints(JsonWriter& json, const CLI::Option* opt) {
            json.key("constraints");
            json.beginArray();

            try {
                const CLI::Validator* validator = const_cast<CLI::Option*>(opt)->get_validator();
                if (validator != nullptr && !validator->get_description().empty()) {
                    json.beginObject();

                    json.key("kind");
                    json.string("opaque");

                    json.key("source");
                    json.string("cli11-validator-description");

                    json.key("description");
                    json.string(validator->get_description());

                    json.endObject();
                }
            } catch (const CLI::OptionNotFound&) {
            }

            json.endArray();
        }

        void writeRelations(JsonWriter& json, const CLI::Option* opt) {
            json.key("relations");
            json.beginObject();

            json.key("needs");
            json.beginArray();
            for (const CLI::Option* need : opt->get_needs()) {
                json.string(optionSingleName(need));
            }
            json.endArray();

            json.key("excludes");
            json.beginArray();
            for (const CLI::Option* exclude : opt->get_excludes()) {
                json.string(optionSingleName(exclude));
            }
            json.endArray();

            json.endObject();
        }

        void writeValue(JsonWriter& json, const CLI::Option* opt) {
            const OptionValueState state = extractValueState(opt);

            json.key("value");
            json.beginObject();

            json.key("apiDefault");
            json.string(state.apiDefault);

            json.key("configured");
            if (state.isConfigured) {
                json.string(state.configured);
            } else {
                json.null();
            }

            json.key("effective");
            json.string(state.effective);

            json.key("source");
            json.string(state.source);

            json.key("isDefault");
            json.boolean(state.isDefault);

            json.key("isConfigured");
            json.boolean(state.isConfigured);

            json.key("isMissingRequired");
            json.boolean(state.isMissingRequired);

            json.endObject();
        }

        void writeOption(JsonWriter& json, const CLI::Option* opt, const std::string& prefix) {
            const std::string key = prefix + optionSingleName(opt);
            const Classification group = classifyOptionGroup(opt->get_group());

            json.beginObject();

            json.key("id");
            json.string(key);

            json.key("key");
            json.string(key);

            json.key("displayName");
            json.string(displayNameForKey(key));

            json.key("description");
            json.string(opt->get_description());

            json.key("configurable");
            json.boolean(opt->get_configurable());

            json.key("configurableSource");
            json.string("cli11");

            json.key("persistent");
            json.boolean(group.kind != "nonpersistent");

            json.key("persistentSource");
            json.string(group.source);

            json.key("required");
            json.boolean(opt->get_required());

            json.key("requiredSource");
            json.string("cli11");

            json.key("disabled");
            json.boolean(false);

            writeCommandLine(json, opt, key);
            writeConfigFile(json, opt, key);
            writeType(json, opt, key);
            writeConstraints(json, opt);
            writeRelations(json, opt);
            writeValue(json, opt);

            json.endObject();
        }

        void writeOptionGroup(JsonWriter& json,
                              const std::string& group,
                              const std::vector<const CLI::Option*>& options,
                              const std::string& prefix) {
            const Classification classification = classifyOptionGroup(group);

            json.beginObject();

            json.key("name");
            json.string(group);

            json.key("kind");
            json.string(classification.kind);

            json.key("kindSource");
            json.string(classification.source);

            json.key("options");
            json.beginArray();
            for (const CLI::Option* option : options) {
                writeOption(json, option, prefix);
            }
            json.endArray();

            json.endObject();
        }

        void writeOptionGroups(JsonWriter& json, const CLI::App* app, const std::string& prefix) {
            std::vector<std::string> groups = app->get_groups();
            bool defaultUsed = false;

            groups.insert(groups.begin(), "Options");

            json.key("optionGroups");
            json.beginArray();

            for (const std::string& group : groups) {
                if ((group == "Options" || group.empty()) && defaultUsed) {
                    continue;
                }
                if (group == "Options" || group.empty()) {
                    defaultUsed = true;
                }

                std::vector<const CLI::Option*> options;
                for (const CLI::Option* option : app->get_options({})) {
                    if (!option->get_configurable()) {
                        continue;
                    }
                    if (option->get_group() == group || (group == "Options" && option->get_group().empty())) {
                        options.push_back(option);
                    }
                }

                if (!options.empty()) {
                    writeOptionGroup(json, group, options, prefix);
                }
            }

            json.endArray();
        }

        void writeNode(JsonWriter& json,
                       const CLI::App* app,
                       const std::vector<std::string>& path,
                       const std::string& prefix,
                       bool root) {
            const Classification node = classifyNode(app, root);

            json.beginObject();

            json.key("id");
            json.string(root ? "app" : CLI::detail::join(path, "."));

            json.key("kind");
            json.string(node.kind);

            json.key("kindSource");
            json.string(node.source);

            json.key("name");
            json.string(app->get_name());

            json.key("displayName");
            if (root) {
                json.string("Application");
            } else if (app->get_display_name(false).empty()) {
                json.string(app->get_name());
            } else {
                json.string(app->get_display_name(false));
            }

            json.key("group");
            json.string(app->get_group());

            json.key("description");
            json.string(app->get_description());

            json.key("configurable");
            json.boolean(app->get_configurable());

            json.key("configurableSource");
            json.string("cli11");

            json.key("required");
            json.boolean(app->get_required());

            json.key("requiredSource");
            json.string("cli11");

            json.key("disabled");
            json.boolean(app->get_disabled());

            json.key("disabledSource");
            json.string("cli11");

            json.key("path");
            writeStringArray(json, path);

            writeOptionGroups(json, app, prefix);

            json.key("children");
            json.beginArray();
            for (const CLI::App* subcommand : app->get_subcommands({})) {
                std::vector<std::string> childPath = path;
                std::string childPrefix = prefix;

                if (!subcommand->get_name().empty()) {
                    childPath.push_back(subcommand->get_name());
                    childPrefix += subcommand->get_name() + ".";
                }

                writeNode(json, subcommand, childPath, childPrefix, false);
            }
            json.endArray();

            json.endObject();
        }

    } // namespace

    std::string ConfigJsonFormatter::toConfig(const CLI::App* app) const {
        std::stringstream out;
        JsonWriter json(out);

        json.beginObject();

        json.key("format");
        json.beginObject();

        json.key("name");
        json.string("snodec.config");

        json.key("version");
        json.number("1");

        json.key("scope");
        json.string("configurable-options-only");

        json.endObject();

        json.key("application");
        json.beginObject();

        json.key("name");
        json.string(app->get_name());

        json.key("description");
        json.string(app->get_description());

        json.key("version");
        json.string(app->version());

        json.endObject();

        json.key("tree");
        writeNode(json, app, {}, "", true);

        json.endObject();
        out << '\n';

        return out.str();
    }

} // namespace utils
