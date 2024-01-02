/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
    class Formatter;
} // namespace CLI

#include <map>
#include <memory>
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
        static bool bootstrap();
        static void terminate();

        static CLI::App* add_instance(const std::string& name, const std::string& description, const std::string& group);
        static bool remove_instance(CLI::App* instance);

        static void required(CLI::App* instance, bool req = true);

        static CLI::App* add_standard_flags(CLI::App* app);
        static CLI::App* add_help_flag(CLI::App* app);
        static CLI::App* add_help_flags(CLI::App* app);

        static std::string getApplicationName();
        static int getLogLevel();
        static int getVerboseLevel();

        static CLI::Option* add_string_option(const std::string& name, const std::string& description, const std::string& typeName);

        static CLI::Option*
        add_string_option(const std::string& name, const std::string& description, const std::string& typeName, bool configurable);

        static CLI::Option* add_string_option(const std::string& name,
                                              const std::string& description,
                                              const std::string& typeName,
                                              const std::string& defaultValue);

        static CLI::Option* add_string_option(const std::string& name,
                                              const std::string& description,
                                              const std::string& typeName,
                                              const std::string& defaultValue,
                                              bool configurable);

        static CLI::Option*
        add_string_option(const std::string& name, const std::string& description, const std::string& typeName, const char* defaultValue);

        static CLI::Option* add_string_option(const std::string& name,
                                              const std::string& description,
                                              const std::string& typeName,
                                              const char* defaultValue,
                                              bool configurable);

        static std::string get_string_option_value(const std::string& name);

        static void add_flag(const std::string& name,
                             bool& variable,
                             const std::string& description,
                             bool required,
                             bool configurable = true,
                             const std::string& groupName = "Application Options");

    private:
        static bool parse1();
        static bool parse2();

        static std::shared_ptr<CLI::App> app;
        static std::shared_ptr<CLI::Formatter> sectionFormatter;

        static int argc;
        static char** argv;

        static std::string applicationName;

        static std::string configDirectory;
        static std::string logDirectory;
        static std::string pidDirectory;

        static CLI::Option* daemonizeOpt;
        static CLI::Option* logFileOpt;
        static CLI::Option* userNameOpt;
        static CLI::Option* groupNameOpt;
        static CLI::Option* enforceLogFileOpt;
        static CLI::Option* logLevelOpt;
        static CLI::Option* verboseLevelOpt;
        static CLI::Option* quietOpt;

        static std::map<std::string, std::string> aliases;             // from -> to
        static std::map<std::string, CLI::Option*> applicationOptions; // keep all user options in memory
    };

} // namespace utils

#endif // UTILS_CONFIG_H
