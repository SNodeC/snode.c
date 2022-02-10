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

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <ostream>   // for ofstream, basic_ostream
#include <stdexcept> // for invalid_argument, out_of_range

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CONFFILEPATH std::string("/home/voc/etc/snode.c")
#define LOGFILEPATH std::string("/home/voc/etc/snode.c")
#define PIDFILEPATH std::string("/home/voc/etc/snode.c")

namespace utils {

    int Config::argc = 0;
    char** Config::argv = nullptr;
    CLI::App Config::app;
    std::string Config::name;
    bool Config::_dumpConfig = false;
    bool Config::_daemonize = false;
    bool Config::_kill = false;
    bool Config::_forceLogFile = false;
    bool Config::_showConfig = false;
    std::string Config::_logFile;
    std::string Config::outputConfigFile;

    int Config::init(int argc, char* argv[]) {
        Config::argc = argc;
        Config::argv = argv;

        name = std::filesystem::path(argv[0]).filename();

        logger::Logger::init(argc, argv);

        app.option_defaults()->take_first();

        app.description("Configuration for application " + name);
        app.allow_extras();
        app.allow_config_extras();

        CLI::Option* allHelpOpt = app.set_help_all_flag("--help-all", "Expand all help");
        allHelpOpt->configurable(false);

        CLI::Option* showConfigFlag = app.add_flag("-s,--show-config", _showConfig, "Show current configuration and exit");
        showConfigFlag->configurable(false);

        CLI::Option* dumpConfigFlg =
            app.add_flag("-w,--write-config{" + CONFFILEPATH + "/" + name + ".conf" + "}", outputConfigFile, "Write config file");
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

        CLI::Option* daemonizeOpt = app.add_flag("-d,!-f,--daemonize,!--foreground", _daemonize, "Start application as daemon");
        daemonizeOpt->excludes(showConfigFlag);
        daemonizeOpt->configurable();

        CLI::Option* killDaemonOpt = app.add_flag("-k,--kill", _kill, "Kill running daemon");
        killDaemonOpt->disable_flag_override();
        killDaemonOpt->configurable(false);

        parse();

        if (_kill) {
            utils::Daemon::kill(PIDFILEPATH + "/" + name + ".pid");
            VLOG(0) << "Daemon killed";

            exit(0);
        }

        if (app["--help"]->count() == 0 && app["--help-all"]->count() == 0) {
            app.set_config("--config", CONFFILEPATH + "/" + name + ".conf", "Read an config file", false);
        }

        parse(); // for daemonize, logfile and forceLogFile

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
        parse(app["--help"]->count() > 0 || app["--help-all"]->count() > 0);

        if (app["--write-config"]->count() > 0) {
            VLOG(0) << "Write config file";
            std::ofstream confFile(outputConfigFile);
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

    const std::string Config::getLogFile() {
        return _logFile;
    }

    bool Config::daemonize() {
        return _daemonize;
    }

    bool Config::kill() {
        return _kill;
    }

    CLI::App* Config::add_subcommand(const std::string& subcommand_name, const std::string& subcommand_description) {
        return app.add_subcommand(subcommand_name, subcommand_description);
    }

    std::string Config::getApplicationName() {
        return name;
    }

    int Config::parse(bool stopOnError) {
        try {
            int errnotmp = errno;
            Config::app.parse(argc, argv);
            errno = errnotmp;
        } catch (const CLI::ParseError& e) {
            if (stopOnError && !_showConfig) {
                exit(Config::app.exit(e));
            }
        }

        return 0;
    }

} // namespace utils
