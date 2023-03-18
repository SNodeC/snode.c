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

#include "Formatter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <iostream>
#include <set>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace CLI {

    CLI11_INLINE std::string ConfigFormatter::to_config(const App* app,
                                                        bool default_also,
                                                        bool write_description,
                                                        std::string prefix) const { // cppcheck-suppress passedByValue
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

    CLI11_INLINE std::string HelpFormatter::make_help(const App* app, std::string name, AppFormatMode mode) const {
        // This immediately forwards to the make_expanded method. This is done this way so that subcommands can
        // have overridden formatters
        if (mode == AppFormatMode::Sub)
            return make_expanded(app);

        std::stringstream out;
        if ((app->get_name().empty()) && (app->get_parent() != nullptr)) {
            if (app->get_group() != "Subcommands") {
                out << app->get_group() << ':';
            }
        }

        out << make_description(app);
        out << make_usage(app, name);
        out << make_positionals(app);
        out << make_groups(app, mode);
        out << make_subcommands(app, mode);

        out << make_footer(app);

        return out.str();
    }

    CLI11_INLINE std::string HelpFormatter::make_usage(const App* app, std::string name) const {
        std::string usage = app->get_usage();
        if (!usage.empty()) {
            return usage + "\n";
        }

        std::stringstream out;

        out << get_label("Usage") << ":" << (name.empty() ? "" : " ") << name;

        std::vector<std::string> groups = app->get_groups();

        // Print an Options badge if any options exist
        std::vector<const Option*> non_pos_options = app->get_options([](const Option* opt) {
            return opt->nonpositional();
        });
        if (!non_pos_options.empty())
            out << " [" << get_label("OPTIONS") << "]";

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
        if (!app->get_subcommands([](const CLI::App* subc) {
                    return ((!subc->get_disabled()) && (!subc->get_name().empty()));
                })
                 .empty()) {
            out << " ["
                << get_label(app->get_subcommands([](const CLI::App* subc) {
                                    return ((!subc->get_disabled()) && (!subc->get_name().empty()) && subc->get_required());
                                }).size() == 1
                                 ? "SUBCOMMAND"
                                 : "SUBCOMMANDS")
                << "]";
        }

        out << std::endl;

        return out.str();
    }

    CLI11_INLINE std::string HelpFormatter::make_description(const App* app) const {
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

    CLI11_INLINE std::string HelpFormatter::make_expanded(const App* sub) const {
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
        std::string tmp = out.str();

        tmp.pop_back();
        // Indent all but the first line (the name)
        return detail::find_and_replace(tmp, "\n", "\n  "); //  + "\n";
    }

    CLI11_INLINE std::string HelpFormatter::make_subcommand(const App* sub) const {
        std::stringstream out;
        detail::format_help(out,
                            sub->get_display_name(true) + (sub->get_required() ? " " + get_label("REQUIRED") : ""),
                            sub->get_description(),
                            column_width_);
        return out.str();
    }

    CLI11_INLINE std::string HelpFormatter::make_group(std::string group, bool is_positional, std::vector<const Option*> opts) const {
        std::stringstream out;

        out << group << ":\n";
        for (const Option* opt : opts) {
            out << make_option(opt, is_positional);
        }
        out << "\n";

        return out.str();
    }

    CLI11_INLINE std::string HelpFormatter::make_groups(const App* app, AppFormatMode mode) const {
        std::stringstream out;
        std::vector<std::string> groups = app->get_groups();

        // Options
        for (const std::string& group : groups) {
            std::vector<const Option*> opts = app->get_options([app, mode, &group](const Option* opt) {
                return opt->get_group() == group                    // Must be in the right group
                       && opt->nonpositional()                      // Must not be a positional
                       && (mode != AppFormatMode::Sub               // If mode is Sub, then
                           || (app->get_help_ptr() != opt           // Ignore help pointer
                               && app->get_help_all_ptr() != opt)); // Ignore help all pointer
            });
            if (!group.empty() && !opts.empty()) {
                out << make_group(group, false, opts);
            }
        }

        std::string tmp = out.str();
        tmp.pop_back();

        return out.str();
    }

    CLI11_INLINE std::string HelpFormatter::make_option_opts(const Option* opt) const {
        std::stringstream out;

        if (!opt->get_option_text().empty()) {
            out << " " << opt->get_option_text();
        } else {
            if (opt->get_type_size() != 0) {
                if (!opt->get_type_name().empty())
                    out << ((opt->get_items_expected_max() == 0) ? "=" : " ") << get_label(opt->get_type_name());
                if (!opt->get_default_str().empty())
                    out << " [" << opt->get_default_str() << "] ";
                if (opt->get_expected_max() == detail::expected_max_vector_size)
                    out << " ...";
                else if (opt->get_expected_min() > 1)
                    out << " x " << opt->get_expected();

                if (opt->get_required())
                    out << " " << get_label("REQUIRED");
            }
            if (!opt->get_envname().empty())
                out << " (" << get_label("Env") << ":" << opt->get_envname() << ")";
            if (!opt->get_needs().empty()) {
                out << " " << get_label("Needs") << ":";
                for (const Option* op : opt->get_needs())
                    out << " " << op->get_name();
            }
            if (!opt->get_excludes().empty()) {
                out << " " << get_label("Excludes") << ":";
                for (const Option* op : opt->get_excludes())
                    out << " " << op->get_name();
            }
        }
        return out.str();
    }

} // namespace CLI
