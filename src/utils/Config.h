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
} // namespace CLI

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    class Config {
    private:
        Config() = delete;
        Config(const Config&) = delete;
        ~Config() = delete;

        Config& operator=(const Config&) = delete;

    public:
        static int init(int argc, char* argv[]);
        static void prepare();
        static void terminate();

        static CLI::App* add_subcommand(const std::string& subcommand_name, const std::string& subcommand_description);

        static std::string getApplicationName();

        static int parse(bool stopOnError = false);

    private:
        static int argc;
        static char** argv;
        static CLI::App app;
        static std::string name;
        static bool _daemonize;
        static std::string outputConfigFile;
        static bool _dumpConfig;
        static bool _kill;
        static bool _forceLogFile;
        static bool _showConfig;
        static std::string _logFile;
    };

} // namespace utils

#endif // UTILS_CONFIG_H
