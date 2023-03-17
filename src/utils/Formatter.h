/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef _CONFIGFORMATER_H
#define _CONFIGFORMATER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp" // IWYU pramga: export

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace CLI {

    class ConfigFormatter : public ConfigBase {
    public:
        CLI11_INLINE std::string
        to_config(const App* app, bool default_also, bool write_description, std::string prefix) const { // cppcheck-suppress passedByValue
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
                if (write_description && group != "Options" && !group.empty()) {
                    out << '\n' << commentChar << commentLead << group << " Options\n";
                }
                for (const Option* opt : app->get_options({})) {
                    // Only process options that are configurable
                    if (opt->get_configurable()) {
                        if (opt->get_group() != group) {
                            if (!(group == "Options" && opt->get_group().empty())) {
                                continue;
                            }
                        }
                        std::string name = prefix + opt->get_single_name();
                        std::string value =
                            detail::ini_join(opt->reduced_results(), arraySeparator, arrayStart, arrayEnd, stringQuote, characterQuote);

                        bool isDefault = false;
                        if (value.empty() && default_also) {
                            if (!opt->get_default_str().empty()) {
                                value = detail::convert_arg_for_ini(opt->get_default_str(), stringQuote, characterQuote);
                            } else if (opt->get_expected_min() == 0) {
                                value = "false";
                            } else if (opt->get_run_callback_for_default()) {
                                value = "\"\""; // empty string default value
                            } else if (opt->get_required()) {
                                value = "\"<REQUIRED>\"";
                            } else {
                                value = "\"\"";
                            }
                            isDefault = true;
                        }

                        if (!value.empty()) {
                            if (!opt->get_fnames().empty()) {
                                value = opt->get_flag_value(name, value);
                            }
                            if (write_description && opt->has_description()) {
                                out << commentLead << detail::fix_newlines(commentLead, opt->get_description()) << '\n';
                            }
                            if (isDefault) {
                                name = commentChar + name;
                            }
                            out << name << valueDelimiter << value << "\n\n";
                        }
                    }
                }
            }
            if (write_description && !out.str().empty()) {
                out << '\n';
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
                outString = commentChar + commentLead + detail::fix_newlines(commentChar + commentLead, app->get_description()) + '\n';
            }

            return outString + out.str();
        }
    };

    class HelpFormatter : public CLI::Formatter {
        CLI11_INLINE std::string make_description(const App* app) const {
            std::string desc = app->get_description();
            auto min_options = app->get_require_option_min();
            auto max_options = app->get_require_option_max();
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

        CLI11_INLINE std::string make_expanded(const App* sub) const {
            std::stringstream out;
            out << sub->get_display_name(true) + (sub->get_required() ? " " + get_label("REQUIRED") : "") << "\n";

            out << make_description(sub);
            if (sub->get_name().empty() && !sub->get_aliases().empty()) {
                detail::format_aliases(out, sub->get_aliases(), column_width_ + 2);
            }
            out << make_positionals(sub);
            out << make_groups(sub, AppFormatMode::Sub);
            out << make_subcommands(sub, AppFormatMode::Sub);

            // Drop blank spaces
            std::string tmp = detail::find_and_replace(out.str(), "\n\n", "\n");
            tmp = tmp.substr(0, tmp.size() - 1); // cppcheck-suppress uselessCallsSubstr

            // Indent all but the first line (the name)
            return detail::find_and_replace(tmp, "\n", "\n  ") + "\n";
        }

        CLI11_INLINE std::string make_subcommand(const App* sub) const {
            std::stringstream out;
            detail::format_help(out,
                                sub->get_display_name(true) + (sub->get_required() ? " " + get_label("REQUIRED") : ""),
                                sub->get_description(),
                                column_width_);
            return out.str();
        }

        CLI11_INLINE std::string make_subcommands(const App* app, AppFormatMode mode) const {
            std::string out = Formatter::make_subcommands(app, mode);
            if (mode == AppFormatMode::All) {
                out.pop_back();
            }
            return out;
        }
    };

} // namespace CLI

#endif // _CONFIGFORMATER_H
