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

#include "log/Logger.h"

#include <cstdlib>
#include <memory>
#include <ostream>   // for ofstream, basic_ostream
#include <stdexcept> // for invalid_argument, out_of_range

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CONFFILEPATH std::string("/home/voc/etc/snode.c")
#define LOGFILEPATH std::string("/home/voc/etc/snode.c")
#define PIDFILEPATH std::string("/home/voc/etc/snode.c")

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

        logger::Logger::init(argc, argv);

        app.description("Configuration for application " + name);
        app.allow_extras();
        app.allow_config_extras();

        CLI::Option* showConfigFlag = app.add_flag("-s,--show-config", _showConfig, "Show current configuration and exit");
        showConfigFlag->configurable(false);

        CLI::Option* dumpConfigFlg = app.add_flag("-w,--write-config", _dumpConfig, "Write config file");
        dumpConfigFlg->disable_flag_override();
        dumpConfigFlg->configurable(false);

        CLI::Option* logFileOpt = app.add_option("-l,--log-file", _logFile, "Log to file");
        logFileOpt->default_val(LOGFILEPATH + "/" + name + ".log");
        logFileOpt->type_name("[path]");
        logFileOpt->excludes(showConfigFlag);
        logFileOpt->configurable();

        CLI::Option* forceLogFileFlag = app.add_flag(
            "-g,!-n,--force-log-file,!--no-log-file", _forceLogFile, "Force writing logs to file for foureground applications");
        forceLogFileFlag->excludes(showConfigFlag);
        forceLogFileFlag->configurable();

        CLI::Option* allHelpOpt = app.set_help_all_flag("--help-all", "Expand all help");
        allHelpOpt->configurable(false);

        CLI::Option* daemonizeOpt = app.add_flag("-d,!-f,--daemonize,!--foreground", _daemonize, "Start application as daemon");
        daemonizeOpt->excludes(showConfigFlag);
        daemonizeOpt->configurable();

        CLI::Option* killDaemonOpt = app.add_flag("-k,--kill", _kill, "Kill running daemon");
        killDaemonOpt->disable_flag_override();
        killDaemonOpt->configurable(false);

        parse();

        if (app["--help"]->count() == 0 && app["--help-all"]->count() == 0) {
            app.set_config("--config", CONFFILEPATH + "/" + name + ".conf", "Read an config file", false);
        }

        if (_kill) {
            utils::Daemon::kill(PIDFILEPATH + "/" + name + ".pid");
            VLOG(0) << "Daemon killed";

            exit(0);
        }

        if (!_showConfig) {
            if (_daemonize) {
                VLOG(0) << "Running as daemon";

                utils::Daemon::daemonize(PIDFILEPATH + "/" + name + ".pid");
                logger::Logger::quiet();
            } else {
                if (!_forceLogFile) {
                    _logFile = "";
                }

                VLOG(0) << "Running in foureground";
            }

            if (!_logFile.empty()) {
                logger::Logger::logToFile(_logFile);
            }
        }

        return 0;
    }

    void Config::prepare() {
        parse();

        if (_dumpConfig) {
            std::ofstream confFile(app["--config"]->as<std::string>());
            confFile << app.config_to_str(true, true);
        }

        if (_showConfig) {
            VLOG(0) << "Show current configuration\n" << app.config_to_str(true, true);

            exit(0);
        }
    }

    void Config::terminate() {
        if (_daemonize) {
            Daemon::erasePidFile(PIDFILEPATH + "/" + name + ".pid");
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

    int Config::parse(bool stopOnError) {
        try {
            Config::app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            if (stopOnError && !_showConfig) {
                exit(Config::app.exit(e));
            }
        }

        return 0;
    }

} // namespace utils
