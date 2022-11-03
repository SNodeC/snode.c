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
#include <clocale>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <ostream>
#include <pwd.h>
#include <stdexcept>
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
    std::string Config::applicationName;
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

    bool Config::init(int argc, char* argv[]) {
        Config::argc = argc;
        Config::argv = argv;

        std::setlocale(LC_ALL, "en_US.UTF-8");

        applicationName = std::filesystem::path(argv[0]).filename();

        const char* homedir = nullptr;
        if ((homedir = getenv("XDG_CONFIG_HOME")) == nullptr) {
            if ((homedir = getenv("HOME")) == nullptr) {
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
            defaultLogDir,
            (std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec) &
                ~std::filesystem::perms::others_all);

        std::filesystem::create_directories(defaultPidDir);
        std::filesystem::permissions(
            defaultPidDir,
            (std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec) &
                ~std::filesystem::perms::others_all);

        //        app.option_defaults()->take_first();
        app.option_defaults()->configurable();

        app.description("Configuration for application " + applicationName);
        app.allow_extras();
        app.allow_config_extras();

        app.get_formatter()->column_width(40);

        CLI::Option* allHelpOpt =
            app.set_help_all_flag("--help-all", "Expand all help")->group("General Options")->group("General Options");
        allHelpOpt->configurable(false);

        app.get_help_ptr()->group("General Options");

        CLI::Option* showConfigFlag =
            app.add_flag("-s,--show-config", showConfig, "Show current configuration and exit")->group("General Options");
        showConfigFlag->disable_flag_override();
        showConfigFlag->configurable(false);

        CLI::Option* dumpConfigFlg = app.add_flag("-w{" + defaultConfDir + "/" + applicationName + ".conf" + "},--write-config{" +
                                                      defaultConfDir + "/" + applicationName + ".conf" + "}",
                                                  outputConfigFile,
                                                  "Write config file");
        dumpConfigFlg->group("General Options");
        dumpConfigFlg->configurable(false);

        CLI::Option* logFileOpt = app.add_option("-l,--log-file", logFile, "Log to file")->group("General Options");
        logFileOpt->default_val(defaultLogDir + "/" + applicationName + ".log");
        logFileOpt->type_name("[path]");
        logFileOpt->excludes(showConfigFlag);

        CLI::Option* startDaemonOpt = app.add_flag("-d,!-f,--daemonize,!--foreground", startDaemon, "Start application as daemon");
        startDaemonOpt->group("General Options");
        startDaemonOpt->excludes(showConfigFlag);

        CLI::Option* stopDaemonOpt = app.add_flag("-k,--kill", stopDaemon, "Kill running daemon");
        stopDaemonOpt->group("General Options");
        stopDaemonOpt->disable_flag_override();
        stopDaemonOpt->configurable(false);

        CLI::Option* forceLogFileFlag =
            app.add_flag("-e,--enforce-log-file", forceLogFile, "Enforce writing logs to file for foureground applications");
        forceLogFileFlag->group("General Options");
        forceLogFileFlag->excludes(showConfigFlag);

        app.set_config("-c,--config", defaultConfDir + "/" + applicationName + ".conf", "Read an config file", false)
            ->group("General Options");

        bool ret = parse(); // for stopDaemon but do not act on -h or --help-all

        if (stopDaemon) {
            utils::Daemon::stopDaemon(defaultPidDir + "/" + applicationName + ".pid");

            ret = false;
        }

        return ret;
    }

    bool Config::prepare() {
        bool ret = parse(true);

        if (ret) {
            if (showConfig) {
                VLOG(0) << "Show current configuration\n" << app.config_to_str(true, true);

                ret = false;
            } else {
                if (startDaemon) {
                    VLOG(0) << "Running as daemon";

                    ret = utils::Daemon::startDaemon(defaultPidDir + "/" + applicationName + ".pid");

                    if (ret) {
                        logger::Logger::quiet();
                    }
                } else if (forceLogFile) {
                    logger::Logger::logToFile(logFile);
                }

                if (app["--write-config"]->count() > 0) {
                    VLOG(0) << "Write config file " << outputConfigFile;
                    std::ofstream confFile(outputConfigFile);
                    confFile << app.config_to_str(true, true);

                    ret = false;
                } else {
                    VLOG(0) << "Running in foureground";
                }
            }
        }

        return ret;
    }

    void Config::terminate() {
        if (startDaemon) {
            Daemon::erasePidFile(defaultPidDir + "/" + applicationName + ".pid");
        }

        if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) >= 0) {
            char buf[1024];
            while (read(STDIN_FILENO, buf, 1024) > 0)
                ;
        }
    }

    CLI::App* Config::add_subcommand(const std::string& subcommand_name, const std::string& subcommand_description) {
        return app.add_subcommand(subcommand_name, subcommand_description);
    }

    void Config::add_option(const std::string& name,
                            std::string& variable,
                            const std::string& description,
                            bool required,
                            const std::string& typeName,
                            const std::string& default_val,
                            bool configurable,
                            const std::string& groupName) {
        add_option(name, variable, description)
            ->required(required)
            ->default_val(default_val)
            ->type_name(typeName)
            ->configurable(configurable)
            ->group(groupName);
    }

    void Config::add_option(const std::string& name,
                            int& variable,
                            const std::string& description,
                            bool required,
                            const std::string& typeName,
                            int default_val,
                            bool configurable,
                            const std::string& groupName) {
        add_option(name, variable, description)
            ->required(required)
            ->default_val(default_val)
            ->type_name(typeName)
            ->configurable(configurable)
            ->group(groupName);
    }

    void Config::add_flag(const std::string& name,
                          bool& variable,
                          const std::string& description,
                          bool required,
                          bool configurable,
                          const std::string& groupName) {
        add_flag(name, variable, description)->required(required)->configurable(configurable)->group(groupName);
    }

    CLI::Option* Config::add_option(const std::string& name, int& variable, const std::string& description) {
        return app.add_option(name, variable, description);
    }

    CLI::Option* Config::add_option(const std::string& name, std::string& variable, const std::string& description) {
        return app.add_option(name, variable, description);
    }

    CLI::Option* Config::add_flag(const std::string& name, const std::string& description) {
        return app.add_flag(name, description);
    }

    CLI::Option* Config::add_flag(const std::string& name, bool& variable, const std::string& description) {
        return app.add_flag(name, variable, description);
    }

    std::string Config::getApplicationName() {
        return applicationName;
    }

    bool Config::parse(bool stopOnError) {
        bool ret = true;

        if (stopOnError) {
            Config::app.allow_extras(false);
        }

        try {
            Config::app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            if (stopOnError && !showConfig && app["--write-config"]->count() == 0) {
                Config::app.exit(e);

                ret = false;
            }
        }

        return ret;
    }

} // namespace utils
