/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef UTILS_CONFIG_H
#define UTILS_CONFIG_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    class Config {
    public:
        Config() = delete;
        Config(const Config&) = delete;
        ~Config() = delete;

        Config& operator=(const Config&) = delete;

        static bool init(int argc, char* argv[]);
        static bool prepare();
        static void terminate();

        static CLI::App* add_subcommand(const std::string& subcommand_name, const std::string& subcommand_description);

        static bool remove_subcommand(CLI::App* subCommand);

        static void add_option(const std::string& name,
                               std::string& variable,
                               const std::string& description,
                               bool required,
                               const std::string& typeName = "[string]",
                               const std::string& default_val = "",
                               bool configurable = true,
                               const std::string& groupName = "Application Options");

        static void add_option(const std::string& name,
                               int& variable,
                               const std::string& description,
                               bool required,
                               const std::string& typeName = "[int]",
                               int default_val = 0,
                               bool configurable = true,
                               const std::string& groupName = "Application Options");

        static void add_flag(const std::string& name,
                             bool& variable,
                             const std::string& description,
                             bool required,
                             bool configurable = true,
                             const std::string& groupName = "Application Options");

        static std::string getApplicationName();

        static bool parse(bool stopOnError = false);

    private:
        static CLI::Option* add_option(const std::string& name, int& variable, const std::string& description);
        static CLI::Option* add_option(const std::string& name, std::string& variable, const std::string& description);
        static CLI::Option* add_flag(const std::string& name, const std::string& description = "");
        static CLI::Option* add_flag(const std::string& name, bool& variable, const std::string& description = "");

    private:
        static int argc;
        static char** argv;
        static CLI::App app;
        static std::string applicationName;
        static bool startAsDaemon;
        static std::string outputConfigFile;
        static std::string logFile;
        static std::string defaultConfDir;
        static std::string defaultLogDir;
        static std::string defaultPidDir;
    };

} // namespace utils

#endif // UTILS_CONFIG_H
