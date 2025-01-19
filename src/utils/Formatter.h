/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef UTILS_FORMATER_H
#define UTILS_FORMATER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#include "utils/CLI11.hpp"
#pragma GCC diagnostic pop
#endif

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {

    class ConfigFormatter : public ConfigBase {
    public:
        ~ConfigFormatter() override;

    private:
        std::string to_config(const App* app, bool default_also, bool write_description, std::string prefix) const override;
    };

    class HelpFormatter : public CLI::Formatter {
    public:
        ~HelpFormatter() override;

    private:
        std::string make_group(std::string group, bool is_positional, std::vector<const Option*> opts) const override;
        std::string make_description(const App* app) const override;
        std::string make_usage(const App* app, std::string name) const override;
        std::string make_subcommands(const App* app, AppFormatMode mode) const override;
        std::string make_subcommand(const App* sub) const override;
        std::string make_expanded(const App* sub) const override;
        std::string make_option_opts(const Option* opt) const override;
    };

} // namespace CLI

#endif // UTILS_FORMATER_H
