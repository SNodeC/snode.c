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

#include "utils/Config.h"

#include "utils/CLI11.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdlib>   // for exit
#include <iostream>  // for basic_ostream, endl, operator<<, cout, ofstream, ostream
#include <stdexcept> // for invalid_argument, out_of_range

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CONFFILEPATH "/home/voc/etc/snode.c"

namespace utils {

    CLI::App Config::app;

    Config::Config() {
    }

    Config::~Config() {
    }

    Config& Config::instance() {
        static Config config;

        return config;
    }

    int Config::init(const std::string& name, int argc, char* argv[]) {
        this->name = name;
        this->argc = argc;
        this->argv = argv;

        app.description("Configuration file for application " + name);

        CLI::Option* dumpConfigFlg = app.add_flag("-w,--write-config", _dumpConfig, "Write config file");
        dumpConfigFlg->configurable(false);

        CLI::Option* logFileOpt = app.add_option("-l,--log-file", _logFile, "Log to file");
        logFileOpt->type_name("[path]");
        logFileOpt->configurable();

        CLI::Option* allHelpOpt = app.set_help_all_flag("--help-all", "Expand all help");
        allHelpOpt->configurable(false);

        CLI::Option* daemonizeOpt = app.add_flag("-d,!-f,--daemonize,!--foreground", _daemonize, "Start application as daemon");
        daemonizeOpt->configurable();

        CLI::Option* killDaemonOpt = app.add_flag("-k,--kill", _kill, "Kill running daemon");
        killDaemonOpt->configurable(false);

        parse();

        if (app.count("--help") == 0) {
            app.set_config("--config", std::string(CONFFILEPATH) + "/" + name + ".conf", "Read an config file", false);
            parse();
        }

        setRequired(_dumpConfig == false);

        return 0;
    }

    void Config::finish() {
        parse(true);

        if (_dumpConfig) {
            std::cout << "Dumping config file: " << std::string(CONFFILEPATH) + "/" + name + ".conf" << std::endl;
            std::cout << app.config_to_str(true, true) << std::endl;

            std::ofstream confFile(std::string(std::string(CONFFILEPATH) + "/" + name + ".conf"));
            confFile << app.config_to_str(true, true);
        }
    }

    const std::string Config::getLogFile() const {
        return _logFile;
    }

    bool Config::daemonize() const {
        return _daemonize;
    }

    bool Config::kill() const {
        return _kill;
    }

    CLI::App* Config::add_subcommand(const std::string& subcommand_name, const std::string& subcommand_description) {
        return app.add_subcommand(subcommand_name, subcommand_description);
    }

    CLI::App* Config::required(CLI::App* app, bool required) {
        return app->required(required & _required);
    }

    CLI::Option* Config::required(CLI::Option* option, bool required) {
        return option->required(required & _required);
    }

    void Config::setRequired(bool required) {
        _required = required;
    }

    int Config::parse(bool stopOnError) {
        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            if (stopOnError) {
                exit(app.exit(e));
            }
        }

        return 0;
    }

} // namespace utils
