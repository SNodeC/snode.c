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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Formatter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {

    ConfigFormatter::ConfigFormatter() {
        arrayDelimiter(' ');
    }

    ConfigFormatter::~ConfigFormatter() {
    }

    namespace {
        constexpr const char* MetaSchema = "snodec.config.comment-meta";

        std::string jsonEscape(const std::string& input) {
            std::string escaped;
            for (const char ch : input) {
                switch (ch) {
                    case '\\':
                        escaped += "\\\\";
                        break;
                    case '"':
                        escaped += "\\\"";
                        break;
                    case '\b':
                        escaped += "\\b";
                        break;
                    case '\f':
                        escaped += "\\f";
                        break;
                    case '\n':
                        escaped += "\\n";
                        break;
                    case '\r':
                        escaped += "\\r";
                        break;
                    case '\t':
                        escaped += "\\t";
                        break;
                    default:
                        if (static_cast<unsigned char>(ch) < 0x20) {
                            std::ostringstream hex;
                            hex << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
                            escaped += hex.str();
                        } else {
                            escaped.push_back(ch);
                        }
                }
            }
            return escaped;
        }

        std::string quoteJson(const std::string& value) {
            return "\"" + jsonEscape(value) + "\"";
        }

        std::string lower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        void writeMetaBlock(std::ostream& out, const std::string& entity, const std::vector<std::string>& lines) {
            out << "#@ snodec.meta begin " << entity << '\n';
            for (const auto& line : lines) {
                out << "#@ " << line << '\n';
            }
            out << "#@ snodec.meta end " << entity << '\n';
        }

        std::string jsonStringArray(const std::vector<std::string>& values) {
            std::ostringstream out;
            out << '[';
            for (std::size_t i = 0; i < values.size(); ++i) {
                if (i > 0)
                    out << ", ";
                out << quoteJson(values[i]);
            }
            out << ']';
            return out.str();
        }

        std::string optionKey(const std::string& prefix, const Option* opt) {
            return prefix + opt->get_single_name();
        }

        std::vector<std::string> pathWith(const std::vector<std::string>& parentPath, const std::string& segment) {
            auto path = parentPath;
            if (!segment.empty())
                path.push_back(segment);
            return path;
        }

        void writeDocumentMeta(std::ostream& out, const App* app) {
            writeMetaBlock(out,
                           "document",
                           {"{",
                            "  \"schema\": " + quoteJson(MetaSchema) + ",",
                            "  \"version\": 1,",
                            "  \"entity\": \"document\",",
                            "  \"application\": {",
                            "    \"name\": " + quoteJson(app->get_name()) + ",",
                            "    \"description\": " + quoteJson(app->get_description()),
                            "  },",
                            "  \"scope\": \"configurable-options-only\",",
                            "  \"syntax\": \"ini-with-comment-metadata\"",
                            "}"});
        }

        std::pair<std::string, std::string> nodeKind(const App* app) {
            if (app->get_parent() == nullptr)
                return {"application", "root"};
            if (app->get_name().empty())
                return {"anonymous", "cli11-name"};
            if (app->get_group() == "Instances")
                return {"instance", "cli11-group"};
            if (app->get_group() == "Sections")
                return {"section", "cli11-group"};
            if (!app->get_group().empty())
                return {"category", "heuristic-cli11-group"};
            return {"subcommand", "fallback"};
        }

        void writeNodeMeta(std::ostream& out, const App* app, const std::vector<std::string>& path, const std::string& prefix) {
            const auto [kind, kindSource] = nodeKind(app);
            std::string display = app->get_name();
            if (display.empty())
                display = !app->get_group().empty() ? app->get_group()
                                                    : (!app->get_description().empty() ? app->get_description() : "<anonymous>");
            writeMetaBlock(out,
                           "node",
                           {"{",
                            "  \"schema\": " + quoteJson(MetaSchema) + ",",
                            "  \"version\": 1,",
                            "  \"entity\": \"node\",",
                            "  \"kind\": " + quoteJson(kind) + ",",
                            "  \"kindSource\": " + quoteJson(kindSource) + ",",
                            "  \"name\": " + quoteJson(app->get_name()) + ",",
                            "  \"displayName\": " + quoteJson(display) + ",",
                            "  \"path\": " + jsonStringArray(path) + ",",
                            "  \"configPrefix\": " + quoteJson(prefix) + ",",
                            "  \"group\": " + quoteJson(app->get_group()) + ",",
                            "  \"description\": " + quoteJson(app->get_description()) + ",",
                            std::string("  \"configurable\": ") + (app->get_configurable() ? "true," : "false,"),
                            "  \"required\": {",
                            std::string("    \"effective\": ") + (app->get_required() ? "true," : "false,"),
                            "    \"source\": \"cli11-current-state\"",
                            "  },",
                            std::string("  \"disabled\": ") + (app->get_disabled() ? "true" : "false"),
                            "}"});
        }

        std::pair<std::string, std::string> groupKind(const std::string& group) {
            const std::string l = lower(group);
            if (group.empty() || lower(group) == "options")
                return {"default", "cli11-default-group"};
            if (l.find("nonpersistent") != std::string::npos)
                return {"nonpersistent", "heuristic-group-name"};
            if (l.find("persistent") != std::string::npos)
                return {"persistent", "heuristic-group-name"};
            return {"custom", "heuristic-group-name"};
        }

        void writeGroupMeta(std::ostream& out, const std::string& group, const std::vector<std::string>& path) {
            const auto [kind, source] = groupKind(group);
            writeMetaBlock(out,
                           "group",
                           {"{",
                            "  \"schema\": " + quoteJson(MetaSchema) + ",",
                            "  \"version\": 1,",
                            "  \"entity\": \"group\",",
                            "  \"name\": " + quoteJson(group) + ",",
                            "  \"kind\": " + quoteJson(kind) + ",",
                            "  \"kindSource\": " + quoteJson(source) + ",",
                            "  \"nodePath\": " + jsonStringArray(path),
                            "}"});
        }

        std::string typeKind(const Option* opt) {
            const std::string n = lower(opt->get_single_name() + " " + opt->get_type_name());
            if (opt->get_expected_min() == 0)
                return "boolean";
            if (n.find("port") != std::string::npos || n.find("int") != std::string::npos || n.find("number") != std::string::npos)
                return "integer";
            if (n.find("float") != std::string::npos || n.find("double") != std::string::npos)
                return "number";
            return "string";
        }

        std::string semanticFromLiteral(const std::string& literal) {
            if (literal.size() >= 2 && literal.front() == '"' && literal.back() == '"')
                return literal.substr(1, literal.size() - 2);
            return literal;
        }

        void writeRelations(std::vector<std::string>& lines, const std::string& name, const std::set<Option*>& opts, bool comma) {
            lines.push_back("    \"" + name + "\": [");
            std::size_t idx = 0;
            for (const auto* rel : opts) {
                lines.push_back("      " + quoteJson(rel->get_single_name()) + (++idx < opts.size() ? "," : ""));
            }
            lines.push_back(std::string("    ]") + (comma ? "," : ""));
        }

        void writeOptionMeta(std::ostream& out,
                             const Option* opt,
                             const std::string& key,
                             const std::string& group,
                             const std::vector<std::string>& path,
                             const std::string& defaultValue,
                             const std::string& value) {
            const bool explicitValue = opt->count() > 0;
            const bool missingRequired =
                opt->get_required() && (defaultValue == "\"<REQUIRED>\"" || (!explicitValue && opt->get_default_str().empty()));
            const std::string effectiveLiteral = !value.empty() ? value : defaultValue;
            const std::string source =
                explicitValue ? "command-line-or-config"
                              : (missingRequired ? "required-placeholder" : (!defaultValue.empty() ? "cpp-default" : "empty-default"));
            const std::string longName = opt->get_lnames().empty() ? "null" : quoteJson("--" + opt->get_lnames().front());
            const std::string shortName = opt->get_snames().empty() ? "null" : quoteJson("-" + opt->get_snames().front());
            const auto gk = groupKind(group);
            std::vector<std::string> lines = {
                "{",
                "  \"schema\": " + quoteJson(MetaSchema) + ",",
                "  \"version\": 1,",
                "  \"entity\": \"option\",",
                "  \"id\": " + quoteJson(key) + ",",
                "  \"key\": " + quoteJson(key) + ",",
                "  \"localName\": " + quoteJson(opt->get_single_name()) + ",",
                "  \"displayName\": " + quoteJson(opt->get_single_name()) + ",",
                "  \"nodePath\": " + jsonStringArray(path) + ",",
                "  \"group\": " + quoteJson(group) + ",",
                "  \"description\": " + quoteJson(opt->get_description()) + ",",
                "  \"configurable\": true,",
                "  \"required\": {",
                std::string("    \"effective\": ") + (opt->get_required() ? "true," : "false,"),
                "    \"source\": \"cli11-current-state\"",
                "  },",
                std::string("  \"persistent\": ") + (gk.first == "persistent" ? "true," : "false,"),
                "  \"commandLine\": {",
                "    \"long\": " + longName + ",",
                "    \"short\": " + shortName + ",",
                std::string("    \"takesValue\": ") + (opt->get_expected_min() != 0 ? "true," : "false,"),
                std::string("    \"repeatable\": ") + (opt->get_expected_max() > 1 ? "true" : "false"),
                "  },",
                "  \"configFile\": {",
                "    \"key\": " + quoteJson(key) + ",",
                "    \"section\": null,",
                "    \"writable\": true",
                "  },",
                "  \"type\": {",
                "    \"name\": " + quoteJson(opt->get_type_name().empty() ? opt->get_single_name() : opt->get_type_name()) + ",",
                "    \"kind\": " + quoteJson(typeKind(opt)) + ",",
                "    \"kindSource\": \"heuristic-name\",",
                std::string("    \"items\": ") + (opt->get_items_expected_max() > 1 ? "\"list\"" : "\"single\""),
                "  },",
                "  \"constraints\": [],",
                "  \"relations\": {"};
            writeRelations(lines, "needs", opt->get_needs(), true);
            writeRelations(lines, "excludes", opt->get_excludes(), false);
            lines.insert(lines.end(),
                         {"  },",
                          "  \"value\": {",
                          "    \"registrationDefault\": null,",
                          "    \"registrationDefaultSource\": \"not-tracked\",",
                          "    \"cppDefault\": " + (defaultValue.empty() ? "null" : quoteJson(semanticFromLiteral(defaultValue))) + ",",
                          "    \"configured\": " + (explicitValue && !value.empty() ? quoteJson(semanticFromLiteral(value)) : "null") + ",",
                          "    \"effective\": " +
                              (!effectiveLiteral.empty() ? quoteJson(semanticFromLiteral(effectiveLiteral)) : quoteJson("")) + ",",
                          "    \"source\": " + quoteJson(source) + ",",
                          std::string("    \"isCppDefault\": ") + (!explicitValue ? "true," : "false,"),
                          std::string("    \"isExplicitlyConfigured\": ") + (explicitValue ? "true," : "false,"),
                          std::string("    \"isMissingRequired\": ") + (missingRequired ? "true," : "false,"),
                          "    \"registrationDefaultLiteral\": null,",
                          "    \"cppDefaultLiteral\": " + (defaultValue.empty() ? "null" : quoteJson(defaultValue)) + ",",
                          "    \"configuredLiteral\": " + (explicitValue && !value.empty() ? quoteJson(value) : "null") + ",",
                          "    \"effectiveLiteral\": " + (!effectiveLiteral.empty() ? quoteJson(effectiveLiteral) : "null"),
                          "  }",
                          "}"});
            writeMetaBlock(out, "option", lines);
        }

        std::string toConfigWithMeta(const ConfigFormatter* formatter,
                                     const App* app,
                                     bool default_also,
                                     bool write_description,
                                     const std::string& prefix,
                                     const std::vector<std::string>& path,
                                     bool root) {
            std::stringstream out;
            std::string commentLead;
            commentLead.push_back('#');
            commentLead.push_back(' ');
            if (root)
                writeDocumentMeta(out, app);
            writeNodeMeta(out, app, path, prefix);
            if (write_description && !app->get_description().empty()) {
                out << '#' << '#' << commentLead << detail::fix_newlines('#' + commentLead, app->get_description()) << '\n';
            }

            std::vector<std::string> groups = app->get_groups();
            bool defaultUsed = false;
            groups.insert(groups.begin(), std::string("Options"));
            for (const auto& group : groups) {
                if (group == "Options" || group.empty()) {
                    if (defaultUsed)
                        continue;
                    defaultUsed = true;
                }
                bool hasConfigurable = !app->get_options([group](const Option* opt) {
                                               return opt->get_configurable() &&
                                                      (opt->get_group() == group || (group == "Options" && opt->get_group().empty()));
                                           })
                                            .empty();
                if (write_description && hasConfigurable) {
                    writeGroupMeta(out, group, path);
                    if (group != "Options" && !group.empty())
                        out << '\n' << '#' << commentLead << group << "\n";
                }
                for (const Option* opt : app->get_options({})) {
                    if (!opt->get_configurable())
                        continue;
                    if (opt->get_group() != group && !(group == "Options" && opt->get_group().empty()))
                        continue;
                    const std::string name = optionKey(prefix, opt);
                    std::string value;
                    try {
                        value = detail::ini_join(opt->reduced_results(), ' ', '[', ']', '\"', '\'');
                    } catch (CLI::ParseError& e) {
                        value = std::string{"<["} + Color::Code::FG_RED + e.get_name() + Color::Code::FG_DEFAULT + "] " + e.what() + ">";
                    }
                    std::string defaultValue{};
                    if (default_also) {
                        if (!value.empty() && detail::convert_arg_for_ini(opt->get_default_str(), '\"', '\'', true) == "\"" + value + "\"")
                            value.clear();
                        if (!opt->get_default_str().empty()) {
                            defaultValue = detail::convert_arg_for_ini(opt->get_default_str(), '\"', '\'', true);
                            if (defaultValue == "'\"\"'")
                                defaultValue = "";
                        } else if (opt->get_required())
                            defaultValue = "\"<REQUIRED>\"";
                        else if (opt->get_run_callback_for_default())
                            defaultValue = "\"\"";
                        else if (opt->get_expected_min() == 0)
                            defaultValue = "false";
                        else
                            defaultValue = "\"\"";
                    }
                    writeOptionMeta(out, opt, name, group, path, defaultValue, value);
                    if (write_description && opt->has_description() && (default_also || !value.empty()))
                        out << commentLead << detail::fix_newlines(commentLead, opt->get_description()) << '\n';
                    if (default_also && !defaultValue.empty()) {
                        if (defaultValue == value)
                            value.clear();
                        out << '#' << name << '=' << defaultValue << "\n";
                    }
                    if (!value.empty()) {
                        if (!opt->get_fnames().empty())
                            value = opt->get_flag_value(name, value);
                        if (value == "\"default\"")
                            value = "default";
                        out << name << '=' << value << "\n\n";
                    } else
                        out << '\n';
                }
            }
            std::map<const App*, std::string> anonymousSegments;
            std::size_t anon = 0;
            auto subcommands = app->get_subcommands({});
            for (const App* subcom : subcommands)
                if (subcom->get_name().empty())
                    anonymousSegments[subcom] = "<anonymous-" + std::to_string(anon++) + ">";
            for (const App* subcom : subcommands) {
                if (subcom->get_name().empty()) {
                    out << toConfigWithMeta(
                        formatter, subcom, default_also, write_description, prefix, pathWith(path, anonymousSegments[subcom]), false);
                }
            }
            for (const App* subcom : subcommands) {
                if (!subcom->get_name().empty()) {
                    if (subcom->get_configurable() && app->got_subcommand(subcom)) {
                        if (!prefix.empty() || app->get_parent() == nullptr)
                            out << '[' << prefix << subcom->get_name() << "]\n";
                        else {
                            std::string subname = app->get_name() + '.' + subcom->get_name();
                            const auto* p = app->get_parent();
                            while (p->get_parent() != nullptr) {
                                subname = p->get_name() + '.' + subname;
                                p = p->get_parent();
                            }
                            out << '[' << subname << "]\n";
                        }
                        out << toConfigWithMeta(
                            formatter, subcom, default_also, write_description, "", pathWith(path, subcom->get_name()), false);
                    } else {
                        out << toConfigWithMeta(formatter,
                                                subcom,
                                                default_also,
                                                write_description,
                                                prefix + subcom->get_name() + '.',
                                                pathWith(path, subcom->get_name()),
                                                false);
                    }
                }
            }
            return out.str();
        }
    } // namespace

    CLI11_INLINE std::string
    ConfigFormatter::to_config(const App* app, bool default_also, bool write_description, std::string prefix) const {
        const std::vector<std::string> path =
            app->get_name().empty() ? std::vector<std::string>{} : std::vector<std::string>{app->get_name()};
        return toConfigWithMeta(this, app, default_also, write_description, prefix, path, app->get_parent() == nullptr && prefix.empty());
    }

#ifndef CLI11_ORIGINAL_FORMATTER
    HelpFormatter::HelpFormatter() {
        label("SUBCOMMAND", "SECTION");
        label("SUBCOMMANDS", "SECTIONS");
        label("PERSISTENT", "");
        label("Persistent Options", "Options (persistent)");
        label("Nonpersistent Options", "Options (nonpersistent)");
        label("Usage", "\nUsage");
        label("BOOL:{true,false}", "{true,false}");
        label("bool:{true,false}", "{true,false}");
        label("TRISTAT:{true,false,default}", "{true,false,default}");
        label("MODE:{standard,active,complete,required}", "{standard,active,complete,required}");
        label("MODE:{standard,exact,expanded}", "{standard,exact,expanded}");
        label("MODE:{standard,exact}", "{standard,exact}");

        column_width(7);

        enable_default_flag_values(false);
    }

    HelpFormatter::~HelpFormatter() {
    }

    CLI11_INLINE std::string HelpFormatter::make_usage(const App* app, std::string name) const {
        const std::string usage = app->get_usage();
        if (!usage.empty()) {
            return usage + "\n";
        }

        std::stringstream out;

        out << get_label("Usage") << ":" << (name.empty() ? "" : " ") << name;

        // Print an Options badge if any options exist
        const std::vector<const Option*> non_pos_options = app->get_options([](const Option* opt) {
            return opt->nonpositional();
        });
        if (!non_pos_options.empty()) {
            out << " [" << get_label("OPTIONS") << "]";
        }

        // Positionals need to be listed here
        std::vector<const Option*> positionals = app->get_options([](const Option* opt) {
            return opt->get_positional();
        });

        // Print out positionals if any are left
        if (!positionals.empty()) {
            // Convert to help names
            std::vector<std::string> positional_names(positionals.size());
            std::transform(positionals.begin(), positionals.end(), positional_names.begin(), [this](const Option* opt) {
                return make_option_usage(opt);
            });

            out << " " << detail::join(positional_names, " ");
        }

        // Add a marker if subcommands are expected or optional
        if (!app->get_subcommands([](const App* subc) {
                    return !subc->get_disabled() && !subc->get_name().empty();
                })
                 .empty()) {
            // ############## Next line changed
            out << " ["
                << get_label(app->get_subcommands([](const CLI::App* subc) {
                                    return ((!subc->get_disabled()) && (!subc->get_name().empty()) /*&& subc->get_required()*/);
                                }).size() <= 1
                                 ? "SUBCOMMAND"
                                 : "SUBCOMMANDS")
                << " [--help]"
                << "]";
        }

        const Option* disabledOpt = app->get_option_no_throw("--disabled");
        if (disabledOpt != nullptr ? disabledOpt->as<bool>() : false) {
            out << " " << get_label("DISABLED");
        } else if (app->get_required()) {
            out << " " << get_label("REQUIRED");
        }

        out << std::endl;

        return out.str();
    }

    CLI11_INLINE std::string HelpFormatter::make_footer(const App* app) const {
        const std::string footer = app->get_footer();
        if (footer.empty()) {
            return std::string{"\n"};
        }
        return '\n' + footer + "\n\n";
    }

    CLI11_INLINE std::string HelpFormatter::make_subcommands(const App* app, AppFormatMode mode) const {
        std::stringstream out;

        const std::vector<const App*> subcommands = app->get_subcommands([](const App* subc) {
            if (subc->get_group() == "Instances") {
                if (subc->get_option("--disabled")->as<bool>()) {
                    const_cast<CLI::App*>(subc)->group(subc->get_group() + " (disabled)");
                }
            }
            return !subc->get_disabled() && !subc->get_name().empty();
        });

        // Make a list in alphabetic order of the groups seen
        std::set<std::string> subcmd_groups_seen;
        for (const App* com : subcommands) {
            if (com->get_name().empty()) {
                if (!com->get_group().empty()) {
                    out << make_expanded(com, mode);
                }
                continue;
            }
            std::string group_key = com->get_group();
            if (!group_key.empty() &&
                std::find_if(subcmd_groups_seen.begin(), subcmd_groups_seen.end(), [&group_key](const std::string& a) {
                    return detail::to_lower(a) == detail::to_lower(group_key);
                }) == subcmd_groups_seen.end()) {
                subcmd_groups_seen.insert(group_key);
            }
        }

        // For each group, filter out and print subcommands
        const Option* disabledOpt = app->get_option_no_throw("--disabled");
        bool disabled = false;
        if (disabledOpt != nullptr) {
            disabled = disabledOpt->as<bool>();
        }

        if (!disabled) {
            for (const std::string& group : subcmd_groups_seen) {
                out << "\n" << group << ":\n";
                const std::vector<const App*> subcommands_group = app->get_subcommands([&group](const App* sub_app) {
                    return detail::to_lower(sub_app->get_group()) == detail::to_lower(group) && !sub_app->get_disabled() &&
                           !sub_app->get_name().empty();
                });
                for (const App* new_com : subcommands_group) {
                    if (new_com->get_name().empty()) {
                        continue;
                    }
                    if (mode != AppFormatMode::All) {
                        out << make_subcommand(new_com);
                    } else {
                        out << new_com->help(disabledOpt != nullptr && (app->get_help_ptr()->as<std::string>() == "exact" ||
                                                                        app->get_help_ptr()->as<std::string>() == "expanded")
                                                 ? new_com
                                                 : nullptr,
                                             new_com->get_name(),
                                             AppFormatMode::Sub);
                        out << "\n";
                    }
                }
            }
        }

        // ########## Next line(s) changed

        std::string tmp = out.str();
        if (mode == AppFormatMode::All && !tmp.empty()) {
            tmp.pop_back();
        }

        out.str(tmp);
        out.clear();

        return out.str();
    }

    CLI11_INLINE std::string HelpFormatter::make_subcommand(const App* sub) const {
        std::stringstream out;
        const std::string name = "  " + sub->get_display_name(true) + (sub->get_required() ? " " + get_label("REQUIRED") : "");

        out << std::setw(static_cast<int>(column_width_)) << std::left << name;
        const std::string desc = sub->get_description();
        if (!desc.empty()) {
            bool skipFirstLinePrefix = true;
            if (out.str().length() >= column_width_) {
                out << '\n';
                skipFirstLinePrefix = false;
            }
            detail::streamOutAsParagraph(out, desc, right_column_width_, std::string(column_width_, ' '), skipFirstLinePrefix);
        }

        out << '\n';

        return out.str();
    }

    CLI11_INLINE std::string HelpFormatter::make_expanded(const App* sub, AppFormatMode mode) const {
        std::stringstream out;
        // ########## Next lines changed
        const Option* disabledOpt = sub->get_option_no_throw("--disabled");
        out << sub->get_display_name(true) + " [OPTIONS]" + (!sub->get_subcommands({}).empty() ? " [SECTIONS]" : "") +
                   ((disabledOpt != nullptr ? disabledOpt->as<bool>() : false) ? " " + get_label("DISABLED")
                                                                               : (sub->get_required() ? " " + get_label("REQUIRED") : ""))
            << "\n";

        detail::streamOutAsParagraph(out, make_description(sub), description_paragraph_width_, ""); // Format description as paragraph

        if (sub->get_name().empty() && !sub->get_aliases().empty()) {
            detail::format_aliases(out, sub->get_aliases(), column_width_ + 2);
        }
        out << make_positionals(sub);
        out << make_groups(sub, mode);
        out << make_subcommands(sub, mode);

        // Drop blank spaces
        std::string tmp = out.str();
        tmp.pop_back();

        // Indent all but the first line (the name)
        return detail::find_and_replace(tmp, "\n", "\n  ") + "\n";
    }

    CLI11_INLINE std::string HelpFormatter::make_option_opts(const Option* opt) const {
        std::stringstream out;

        if (!opt->get_option_text().empty()) {
            out << " " << opt->get_option_text();
        } else {
            if (opt->get_type_size() != 0) {
                if (!opt->get_type_name().empty()) {
                    // ########## Next line changed
                    out << ((opt->get_items_expected_max() == 0) ? "=" : " ") << get_label(opt->get_type_name());
                }
                if (!opt->get_default_str().empty()) {
                    out << " [" << opt->get_default_str() << "]";
                }
                try {
                    if (opt->count() > 0 && opt->get_default_str() != opt->as<std::string>()) {
                        out << " {";
                        std::string completeResult;
                        for (const auto& result : opt->reduced_results()) {
                            completeResult += (!result.empty() ? result : "\"\"") + " ";
                        }
                        completeResult.pop_back();
                        out << completeResult << "}";
                    }
                } catch (CLI::ParseError& e) {
                    out << " <[" << Color::Code::FG_RED << e.get_name() << Color::Code::FG_DEFAULT << "] " << e.what() << ">";
                }
                if (opt->get_expected_max() == detail::expected_max_vector_size) {
                    out << " ... ";
                } else if (opt->get_expected_min() > 1) {
                    out << " x " << opt->get_expected();
                }
                if (opt->get_required() && !get_label("REQUIRED").empty()) {
                    out << " " << get_label("REQUIRED");
                }
                if (opt->get_configurable() && !get_label("PERSISTENT").empty()) {
                    out << " " << get_label("PERSISTENT");
                }
            }
            if (!opt->get_envname().empty()) {
                out << " (" << get_label("Env") << ":" << opt->get_envname() << ") ";
            }
            if (!opt->get_needs().empty()) {
                out << " " << get_label("Needs") << ":";
                for (const Option* op : opt->get_needs()) {
                    out << " " << op->get_name();
                }
            }
            if (!opt->get_excludes().empty()) {
                out << " " << get_label("Excludes") << ":";
                for (const Option* op : opt->get_excludes()) {
                    out << " " << op->get_name();
                }
            }
        }
        return out.str();
    }

#endif // CLI11_ORIGINAL_FORMATTER

} // namespace CLI
