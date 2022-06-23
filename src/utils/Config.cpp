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
#include <memory>  // for __shared_ptr_access, shared_ptr
#include <ostream> // for ofstream, basic_ostream
#include <pwd.h>
#include <stdexcept> // for invalid_argument, out_of_range
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef CONFFILEPATH
#define CONFFILEPATH std::string("/etc/snode.c/conf")
#endif

#ifndef LOGFILEPATH
#define LOGFILEPATH std::string("/etc/snode.c/log")
#endif

#ifndef PIDFILEPATH
#define PIDFILEPATH std::string("/etc/snode.c/pid")
#endif

namespace utils {

    int Config::argc = 0;
    char** Config::argv = nullptr;
    CLI::App Config::app;
    std::string Config::name;
    bool Config::dumpConfig = false;
    bool Config::startDaemon = false;
    bool Config::stopDaemon = false;
    bool Config::forceLogFile = false;
    bool Config::showConfig = false;
    std::string Config::logFile;
    std::string Config::outputConfigFile;

    std::string Config::defaultConfDir;
    std::string Config::defaultLogDir;
    std::string Config::defaultPidDir;

    int Config::init(int argc, char* argv[]) {
        Config::argc = argc;
        Config::argv = argv;

        name = std::filesystem::path(argv[0]).filename();

        const char* homedir;
        if ((homedir = getenv("XDG_CONFIG_HOME")) == NULL) {
            if ((homedir = getenv("HOME")) == NULL) {
                homedir = getpwuid(getuid())->pw_dir;
            }
        }

        defaultConfDir = homedir + CONFFILEPATH;
        defaultLogDir = homedir + LOGFILEPATH;
        defaultPidDir = homedir + PIDFILEPATH;

        logger::Logger::init(argc, argv);
        std::filesystem::create_directories(defaultConfDir);
        std::filesystem::permissions(
            defaultConfDir,
            (std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec) &
                ~std::filesystem::perms::others_all);

        std::filesystem::create_directories(defaultLogDir);
        std::filesystem::permissions(
            defaultLogDir, (std::filesystem::perms::owner_all | std::filesystem::perms::group_all) & ~std::filesystem::perms::others_all);

        std::filesystem::create_directories(defaultPidDir);
        std::filesystem::permissions(
            defaultPidDir, (std::filesystem::perms::owner_all | std::filesystem::perms::group_all) & ~std::filesystem::perms::others_all);

        app.option_defaults()->take_first();
        app.option_defaults()->configurable();
        app.configurable();

        app.description("Configuration for application " + name);
        app.allow_extras();
        app.allow_config_extras();

        app.get_formatter()->column_width(40);

        CLI::Option* allHelpOpt = app.set_help_all_flag("--help-all", "Expand all help");
        allHelpOpt->configurable(false);

        CLI::Option* showConfigFlag = app.add_flag("-s,--show-config", showConfig, "Show current configuration and exit");
        showConfigFlag->configurable(false);

        CLI::Option* dumpConfigFlg =
            app.add_flag("-w{" + defaultConfDir + "/" + name + ".conf" + "},--write-config{" + defaultConfDir + "/" + name + ".conf" + "}",
                         outputConfigFile,
                         "Write config file");
        dumpConfigFlg->configurable(false);

        CLI::Option* logFileOpt = app.add_option("-l,--log-file", logFile, "Log to file");
        logFileOpt->default_val(defaultLogDir + "/" + name + ".log");
        logFileOpt->type_name("[path]");
        logFileOpt->excludes(showConfigFlag);

        CLI::Option* forceLogFileFlag =
            app.add_flag("-g,!-n,--force-log-file,!--no-log-file", forceLogFile, "Force writing logs to file for foureground applications");
        forceLogFileFlag->excludes(showConfigFlag);

        CLI::Option* startDaemonOpt = app.add_flag("-d,!-f,--daemonize,!--foreground", startDaemon, "Start application as daemon");
        startDaemonOpt->excludes(showConfigFlag);

        CLI::Option* stopDaemonOpt = app.add_flag("-k,--kill", stopDaemon, "Kill running daemon");
        stopDaemonOpt->disable_flag_override();
        stopDaemonOpt->configurable(false);

        parse();

        if (stopDaemon) {
            utils::Daemon::stopDaemon(defaultPidDir + "/" + name + ".pid");
            VLOG(0) << "Daemon killed";

            exit(0);
        } else {
            if (app["--help"]->count() == 0 && app["--help-all"]->count() == 0) {
                app.set_config("--config", defaultConfDir + "/" + name + ".conf", "Read an config file", false);
            }

            parse(); // for daemonize, logfile and forceLogFile

            if (!showConfig) {
                if (startDaemon) {
                    VLOG(0) << "Running as daemon";

                    utils::Daemon::startDaemon(defaultPidDir + "/" + name + ".pid");
                    logger::Logger::quiet();
                } else {
                    if (!forceLogFile) {
                        logFile = "";
                    }

                    VLOG(0) << "Running in foureground";
                }

                if (!logFile.empty()) {
                    logger::Logger::logToFile(logFile);
                }
            }
        }

        return 0;
    }

    void Config::prepare() {
        parse(app["--help"]->count() > 0 || app["--help-all"]->count() > 0);

        if (app["--write-config"]->count() > 0) {
            VLOG(0) << "Write config file " << outputConfigFile;
            std::ofstream confFile(outputConfigFile);
            confFile << app.config_to_str(true, true);
        }

        if (showConfig) {
            VLOG(0) << "Show current configuration\n" << app.config_to_str(true, true);

            exit(0);
        }
    }

    void Config::terminate() {
        if (startDaemon) {
            Daemon::erasePidFile(defaultPidDir + "/" + name + ".pid");
        }
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
            if (stopOnError && !showConfig) {
                exit(Config::app.exit(e));
            }
        }

        return 0;
    }

} // namespace utils
