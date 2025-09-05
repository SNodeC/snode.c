/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include <algorithm>
#include <functional>
#include <iomanip>
#include <set>
#include <sstream>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {

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
                    std::string value =
                        detail::ini_join(opt->reduced_results(), arraySeparator, arrayStart, arrayEnd, stringQuote, literalQuote);

                    std::string defaultValue{};
                    if (default_also) {
                        static_assert(std::string::npos + static_cast<std::string::size_type>(1) == 0,
                                      "std::string::npos + static_cast<std::string::size_type>(1) != 0");
                        if (!value.empty() && detail::convert_arg_for_ini(opt->get_default_str(), stringQuote, literalQuote) == value) {
                            value.clear();
                        }
                        if (!opt->get_default_str().empty()) {
                            defaultValue = detail::convert_arg_for_ini(opt->get_default_str(), stringQuote, literalQuote);
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

    HelpFormatter::~HelpFormatter() {
    }

    CLI11_INLINE std::string HelpFormatter::make_group(std::string group, bool is_positional, std::vector<const Option*> opts) const {
        std::stringstream out;

        // ############## Next line changed
        out << "\n" << group << ":\n";
        for (const Option* opt : opts) {
            out << make_option(opt, is_positional);
        }

        return out.str();
    }

    CLI11_INLINE std::string HelpFormatter::make_description(const App* app) const {
        std::string desc = app->get_description();
        auto min_options = app->get_require_option_min();
        auto max_options = app->get_require_option_max();
        // ############## Next line changed (if statement removed)
        if ((max_options == min_options) && (min_options > 0)) {
            if (min_options == 1) {
                desc += " \n[Exactly 1 of the following options is required]";
            } else {
                desc += " \n[Exactly " + std::to_string(min_options) + " options from the following list are required]";
            }
        } else if (max_options > 0) {
            if (min_options > 0) {
                desc += " \n[Between " + std::to_string(min_options) + " and " + std::to_string(max_options) +
                        " of the follow options are required]";
            } else {
                desc += " \n[At most " + std::to_string(max_options) + " of the following options are allowed]";
            }
        } else if (min_options > 0) {
            desc += " \n[At least " + std::to_string(min_options) + " of the following options are required]";
        }
        return (!desc.empty()) ? desc + "\n" : std::string{};
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

        out << std::endl << std::endl;

        return out.str();
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
        std::string name = "  " + sub->get_display_name(true) + (sub->get_required() ? " " + get_label("REQUIRED") : "");

        out << std::setw(static_cast<int>(column_width_)) << std::left << name;
        std::string desc = sub->get_description();
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
                if (opt->count() > 0 && opt->get_default_str() != opt->as<std::string>()) {
                    out << " {" << opt->as<std::string>() << "}";
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

} // namespace CLI
