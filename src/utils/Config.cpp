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
#include "utils/Daemon.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>  // for exit
#include <iostream> // for basic_ostream, endl, operator<<, cout, ofstream, ostream
#include <poll.h>
#include <stdexcept> // for invalid_argument, out_of_range
#include <stdlib.h>
#include <sys/types.h>
#include <syscall.h>
#include <unistd.h>

#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434 /* System call # on most architectures */
#endif

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

        app.description("Configuration for application " + name);
        app.allow_extras();

        CLI::Option* dumpConfigFlg = app.add_flag("-w,--write-config", _dumpConfig, "Write config file");
        dumpConfigFlg->configurable(false);

        CLI::Option* logFileOpt = app.add_option("-l,--log-file", _logFile, "Log to file");
        logFileOpt->type_name("[path]");
        logFileOpt->configurable();

        CLI::Option* allHelpOpt = app.set_help_all_flag("--help-all", "Expand all help");
        allHelpOpt->configurable(false);

        CLI::Option* configFileOpt = app.add_option("-c,--config-file", configFile, "Config file");
        configFileOpt->type_name("[path]");
        configFileOpt->default_val(std::string(CONFFILEPATH) + "/" + name + ".conf");

        CLI::Option* daemonizeOpt = app.add_flag("-d,!-f,--daemonize,!--foreground", _daemonize, "Start application as daemon");
        daemonizeOpt->configurable();

        CLI::Option* killDaemonOpt = app.add_flag("-k,--kill", _kill, "Kill running daemon");
        killDaemonOpt->configurable(false);

        parse();

        app.set_config("--config", configFile, "Read an config file", false);

        app.allow_extras(false);

        if (_kill) {
            utils::Daemon::kill("/home/voc/etc/snode.c/" + name + ".pid");
            std::cout << "Daemon killed" << std::endl;

            exit(0);
        }

        if (_daemonize) {
            std::cout << "Daemonizing" << std::endl;
            utils::Daemon::daemonize("/home/voc/etc/snode.c/" + name + ".pid");
        } else {
            std::cout << "Not daemonizing" << std::endl;
        }

        return 0;
    }

    void Config::prepare() {
        parse();

        if (_dumpConfig) {
            std::cout << "Dumping config file: " << app["--config"]->as<std::string>() << std::endl;
            std::cout << app.config_to_str(true, true) << std::endl;

            std::ofstream confFile(app["--config"]->as<std::string>());
            confFile << app.config_to_str(true, true);
        }

        parse(true);
    }

    void Config::terminate() {
        if (_daemonize) {
            Daemon::erasePidFile("/home/voc/etc/snode.c/" + name + ".pid");
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

    int Config::parse([[maybe_unused]] CLI::App* sc, bool stopOnError) {
        try {
            // sc->parse(argc, argv);
            Config::app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            if (stopOnError) {
                exit(Config::app.exit(e));
            }
        }

        return 0;
    }

    int Config::parse(bool stopOnError) {
        return parse(&app, stopOnError);
    }

} // namespace utils
