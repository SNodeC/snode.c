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
        Config();
        ~Config();

    public:
        static Config& instance();
        int init(const std::string& name, int argc, char* argv[]);

        void finish();

        const std::string getLogFile() const;

        bool daemonize() const;

        bool kill() const;

        CLI::App* add_subcommand(const std::string& subcommand_name, const std::string& subcommand_description);

        int parse(CLI::App* sc, bool stopOnError = false);

        int parse(bool stopOnError = false);

    private:
        std::string name;

        std::string configFile;

        static CLI::App app;

        int argc = 0;
        char** argv = nullptr;

        bool _dumpConfig = false;
        bool _daemonize = false;
        bool _kill = false;
        std::string _logFile = "";
    };

} // namespace utils

#endif // UTILS_CONFIG_H
