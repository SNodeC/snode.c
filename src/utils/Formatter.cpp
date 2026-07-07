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
#include <functional>
#include <iomanip>
#include <set>
#include <sstream>
#include <cctype>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {


    JsonWriter::JsonWriter(std::ostream& out)
        : out(out) {
    }

    void JsonWriter::commaIfNeeded() {
        if (!stack.empty()) {
            if (!stack.back().second) {
                out << ',';
            }
            stack.back().second = false;
        }
    }

    void JsonWriter::beforeValue() {
        if (afterKey) {
            afterKey = false;
        } else {
            commaIfNeeded();
        }
    }

    void JsonWriter::beginObject() { beforeValue(); out << '{'; stack.emplace_back(Container::Object, true); }
    void JsonWriter::endObject() { out << '}'; stack.pop_back(); }
    void JsonWriter::beginArray() { beforeValue(); out << '['; stack.emplace_back(Container::Array, true); }
    void JsonWriter::endArray() { out << ']'; stack.pop_back(); }

    void JsonWriter::key(std::string_view value) {
        commaIfNeeded();
        writeEscaped(value);
        out << ':';
        afterKey = true;
    }

    void JsonWriter::string(std::string_view value) { beforeValue(); writeEscaped(value); }
    void JsonWriter::number(std::string_view value) { beforeValue(); out << value; }
    void JsonWriter::boolean(bool value) { beforeValue(); out << (value ? "true" : "false"); }
    void JsonWriter::null() { beforeValue(); out << "null"; }

    void JsonWriter::writeEscaped(std::string_view value) {
        out << '"';
        static constexpr char hex[] = "0123456789abcdef";
        for (const unsigned char ch : value) {
            switch (ch) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (ch < 0x20) {
                        out << "\\u00" << hex[(ch >> 4) & 0x0f] << hex[ch & 0x0f];
                    } else {
                        out << static_cast<char>(ch);
                    }
            }
        }
        out << '"';
    }

    static std::string lowerCopy(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    static std::string displayNameForKey(const std::string& key) {
        const auto pos = key.find_last_of('.');
        return pos == std::string::npos ? key : key.substr(pos + 1);
    }

    static std::string groupKind(const std::string& group) {
        const std::string lower = lowerCopy(group);
        if (group.empty() || group == "Options") return "default";
        if (lower.find("nonpersistent") != std::string::npos) return "nonpersistent";
        if (lower.find("persistent") != std::string::npos) return "persistent";
        return "custom";
    }

    static std::string nodeKind(const App* app, bool root) {
        if (root) return "application";
        if (app->get_name().empty()) return "anonymous";
        if (app->get_group() == "Instances") return "instance";
        if (app->get_group() == "Sections") return "section";
        if (!app->get_group().empty()) return "category";
        return "subcommand";
    }

    static std::string typeKind(const Option* opt, const std::string& key) {
        const std::string t = lowerCopy(opt->get_type_name() + " " + key);
        if (opt->get_items_expected_max() == 0 || t.find("bool") != std::string::npos) return "boolean";
        if (t.find("port") != std::string::npos || t.find("int") != std::string::npos || t.find("level") != std::string::npos) return "integer";
        if (t.find("file") != std::string::npos || t.find("path") != std::string::npos || t.find("config") != std::string::npos) return "path";
        if (t.find('{') != std::string::npos) return "enum";
        return "string";
    }

    static std::string optionValue(const Option* opt) {
        try {
            return detail::ini_join(opt->reduced_results(), ' ', '[', ']', '"', '\'');
        } catch (const CLI::ParseError& e) {
            return std::string{"<"} + e.get_name() + ": " + e.what() + ">";
        }
    }

    static std::string defaultValue(const Option* opt) {
        if (!opt->get_default_str().empty()) return opt->get_default_str();
        if (opt->get_required()) return "<REQUIRED>";
        if (opt->get_expected_min() == 0) return "false";
        return "";
    }

    static void writeStringArray(JsonWriter& json, const std::vector<std::string>& values) {
        json.beginArray();
        for (const auto& value : values) json.string(value);
        json.endArray();
    }

    static std::string optionSingleName(const Option* opt) {
        std::string name = opt->get_single_name();
        if (name.rfind("--", 0) == 0) name.erase(0, 2);
        if (name.rfind('-', 0) == 0) name.erase(0, 1);
        return name;
    }

    static void writeOption(JsonWriter& json, const Option* opt, const std::string& prefix) {
        const std::string key = prefix + optionSingleName(opt);
        const std::string configured = optionValue(opt);
        const bool isConfigured = opt->count() > 0 && !configured.empty();
        const std::string apiDefault = defaultValue(opt);
        const std::string effective = isConfigured ? configured : apiDefault;
        const bool missing = opt->get_required() && (effective.empty() || effective == "<REQUIRED>");
        json.beginObject();
        json.key("id"); json.string(key);
        json.key("key"); json.string(key);
        json.key("displayName"); json.string(displayNameForKey(key));
        json.key("description"); json.string(opt->get_description());
        json.key("configurable"); json.boolean(opt->get_configurable());
        json.key("persistent"); json.boolean(groupKind(opt->get_group()) != "nonpersistent");
        json.key("required"); json.boolean(opt->get_required());
        json.key("disabled"); json.boolean(false);
        json.key("commandLine"); json.beginObject();
        json.key("long"); if (!opt->get_lnames().empty()) json.string("--" + opt->get_lnames().front()); else json.string("--" + key);
        json.key("short"); if (!opt->get_snames().empty()) json.string("-" + opt->get_snames().front()); else json.null();
        json.key("takesValue"); json.boolean(opt->get_items_expected_max() != 0);
        json.key("valueSeparator"); json.string(" ");
        json.key("repeatable"); json.boolean(opt->get_expected_max() > 1);
        json.endObject();
        json.key("configFile"); json.beginObject(); json.key("key"); json.string(key); json.key("section"); json.null(); json.key("writable"); json.boolean(opt->get_configurable()); json.endObject();
        json.key("type"); json.beginObject(); json.key("kind"); json.string(typeKind(opt, key)); json.key("name"); json.string(opt->get_type_name()); json.key("cpp"); json.null(); json.key("items"); json.string(opt->get_items_expected_max() > 1 ? "list" : "single"); json.endObject();
        json.key("constraints"); json.beginArray(); try { if (auto* validator = const_cast<Option*>(opt)->get_validator(); validator != nullptr && !validator->get_description().empty()) { json.beginObject(); json.key("kind"); json.string("opaque"); json.key("description"); json.string(validator->get_description()); json.endObject(); } } catch (const CLI::OptionNotFound&) { } json.endArray();
        json.key("relations"); json.beginObject(); json.key("needs"); json.beginArray(); for (const Option* need : opt->get_needs()) json.string(optionSingleName(need)); json.endArray(); json.key("excludes"); json.beginArray(); for (const Option* ex : opt->get_excludes()) json.string(optionSingleName(ex)); json.endArray(); json.endObject();
        json.key("value"); json.beginObject();
        json.key("apiDefault"); json.string(apiDefault);
        json.key("configured"); if (isConfigured) json.string(configured); else json.null();
        json.key("effective"); json.string(effective);
        json.key("source"); json.string(isConfigured ? "command-line-or-config" : (missing ? "required-placeholder" : (apiDefault.empty() ? "empty-default" : "api-default")));
        json.key("isDefault"); json.boolean(!isConfigured && effective == apiDefault);
        json.key("isConfigured"); json.boolean(isConfigured);
        json.key("isMissingRequired"); json.boolean(missing);
        json.endObject();
        json.endObject();
    }

    static void writeNode(JsonWriter& json, const App* app, const std::vector<std::string>& path, const std::string& prefix, bool root) {
        json.beginObject();
        json.key("id"); json.string(root ? "app" : detail::join(path, "."));
        json.key("kind"); json.string(nodeKind(app, root));
        json.key("name"); json.string(app->get_name());
        json.key("displayName"); json.string(root ? "Application" : (app->get_display_name(false).empty() ? app->get_name() : app->get_display_name(false)));
        json.key("group"); json.string(app->get_group());
        json.key("description"); json.string(app->get_description());
        json.key("configurable"); json.boolean(app->get_configurable());
        json.key("required"); json.boolean(app->get_required());
        json.key("disabled"); json.boolean(app->get_disabled());
        json.key("path"); writeStringArray(json, path);
        json.key("optionGroups"); json.beginArray();
        std::vector<std::string> groups = app->get_groups();
        groups.insert(groups.begin(), "Options");
        bool defaultUsed = false;
        for (const auto& group : groups) {
            if ((group == "Options" || group.empty()) && defaultUsed) continue;
            if (group == "Options" || group.empty()) defaultUsed = true;
            std::vector<const Option*> opts;
            for (const Option* opt : app->get_options({})) {
                if (!opt->get_configurable()) continue;
                if (opt->get_group() == group || (group == "Options" && opt->get_group().empty())) opts.push_back(opt);
            }
            if (opts.empty()) continue;
            json.beginObject(); json.key("name"); json.string(group); json.key("kind"); json.string(groupKind(group)); json.key("options"); json.beginArray();
            for (const Option* opt : opts) writeOption(json, opt, prefix);
            json.endArray(); json.endObject();
        }
        json.endArray();
        json.key("children"); json.beginArray();
        for (const App* subcom : app->get_subcommands({})) {
            std::vector<std::string> childPath = path;
            std::string childPrefix = prefix;
            if (!subcom->get_name().empty()) { childPath.push_back(subcom->get_name()); childPrefix += subcom->get_name() + "."; }
            writeNode(json, subcom, childPath, childPrefix, false);
        }
        json.endArray();
        json.endObject();
    }

    std::string JsonConfigFormatter::to_json_config(const App* app) const {
        std::stringstream out;
        JsonWriter json(out);
        json.beginObject();
        json.key("format"); json.beginObject(); json.key("name"); json.string("snodec.config"); json.key("version"); json.number("1"); json.endObject();
        json.key("application"); json.beginObject(); json.key("name"); json.string(app->get_name()); json.key("description"); json.string(app->get_description()); json.key("version"); json.string(app->version()); json.endObject();
        json.key("tree"); writeNode(json, app, {}, "", true);
        json.endObject();
        out << '\n';
        return out.str();
    }

    ConfigFormatter::ConfigFormatter() {
        arrayDelimiter(' ');
    }

    ConfigFormatter::~ConfigFormatter() {
    }

    CLI11_INLINE std::string
    ConfigFormatter::to_config(const App* app, bool default_also, bool write_description, std::string prefix) const {
        std::stringstream out;
        std::string commentLead;
        commentLead.push_back(commentChar);
        commentLead.push_back(' ');

        std::vector<std::string> groups = app->get_groups();
        bool defaultUsed = false;
        groups.insert(groups.begin(), std::string("Options"));
        for (const auto& group : groups) {
            if (group == "Options" || group.empty()) {
                if (defaultUsed) {
                    continue;
                }
                defaultUsed = true;
            }
            if (write_description && group != "Options" && !group.empty() &&
                !app->get_options([group](const Option* opt) -> bool {
                        return opt->get_group() == group && opt->get_configurable();
                    })
                     .empty()) {
                out << '\n' << commentChar << commentLead << group << "\n";
            }
            for (const Option* opt : app->get_options({})) {
                // Only process options that are configurable
                if (opt->get_configurable()) {
                    if (opt->get_group() != group) {
                        if (!(group == "Options" && opt->get_group().empty())) {
                            continue;
                        }
                    }
                    const std::string name = prefix + opt->get_single_name();
                    std::string value;
                    try {
                        value = detail::ini_join(opt->reduced_results(), arraySeparator, arrayStart, arrayEnd, stringQuote, literalQuote);
                    } catch (CLI::ParseError& e) {
                        value = std::string{"<["} + Color::Code::FG_RED + e.get_name() + Color::Code::FG_DEFAULT + "] " + e.what() + ">";
                    }

                    std::string defaultValue{};
                    if (default_also) {
                        static_assert(std::string::npos + static_cast<std::string::size_type>(1) == 0,
                                      "std::string::npos + static_cast<std::string::size_type>(1) != 0");
                        if (!value.empty() &&
                            detail::convert_arg_for_ini(opt->get_default_str(), stringQuote, literalQuote, true) == "\"" + value + "\"") {
                            value.clear();
                        }
                        if (!opt->get_default_str().empty()) {
                            defaultValue = detail::convert_arg_for_ini(opt->get_default_str(), stringQuote, literalQuote, true);
                            if (defaultValue == "'\"\"'") {
                                defaultValue = "";
                            }
                        } else if (opt->get_run_callback_for_default()) {
                            defaultValue = "\"\""; // empty string default value
                        } else if (opt->get_required()) {
                            defaultValue = "\"<REQUIRED>\"";
                        } else if (opt->get_expected_min() == 0) {
                            defaultValue = "false";
                        } else {
                            defaultValue = "\"\"";
                        }
                    }
                    if (write_description && opt->has_description() && (default_also || !value.empty())) {
                        out << commentLead << detail::fix_newlines(commentLead, opt->get_description()) << '\n';
                    }
                    if (default_also && !defaultValue.empty()) {
                        if (defaultValue == value) {
                            value.clear();
                        }
                        out << commentChar << name << valueDelimiter << defaultValue << "\n";
                    }
                    if (!value.empty()) {
                        if (!opt->get_fnames().empty()) {
                            value = opt->get_flag_value(name, value);
                        }
                        if (value == "\"default\"") {
                            value = "default";
                        }
                        out << name << valueDelimiter << value << "\n\n";
                    } else {
                        out << '\n';
                    }
                }
            }
        }
        auto subcommands = app->get_subcommands({});
        for (const App* subcom : subcommands) {
            if (subcom->get_name().empty()) {
                if (write_description && !subcom->get_group().empty()) {
                    out << '\n' << commentLead << subcom->get_group() << " Options\n";
                }
                out << to_config(subcom, default_also, write_description, prefix);
            }
        }

        for (const App* subcom : subcommands) {
            if (!subcom->get_name().empty()) {
                if (subcom->get_configurable() && app->got_subcommand(subcom)) {
                    if (!prefix.empty() || app->get_parent() == nullptr) {
                        out << '[' << prefix << subcom->get_name() << "]\n";
                    } else {
                        std::string subname = app->get_name() + parentSeparatorChar + subcom->get_name();
                        const auto* p = app->get_parent();
                        while (p->get_parent() != nullptr) {
                            subname = p->get_name() + parentSeparatorChar + subname;
                            p = p->get_parent();
                        }
                        out << '[' << subname << "]\n";
                    }
                    out << to_config(subcom, default_also, write_description, "");
                } else {
                    out << to_config(subcom, default_also, write_description, prefix + subcom->get_name() + parentSeparatorChar);
                }
            }
        }

        std::string outString;
        if (write_description && !out.str().empty()) {
            outString =
                commentChar + std::string("#") + commentLead + detail::fix_newlines(commentChar + commentLead, app->get_description());
        }

        return outString + out.str();
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
