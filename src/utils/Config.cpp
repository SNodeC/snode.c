/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include <clocale>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <memory>
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
    std::string Config::logFile;
    std::string Config::outputConfigFile;
    std::string Config::defaultConfDir;
    std::string Config::defaultLogDir;
    std::string Config::defaultPidDir;

    int Config::logLevel = 0; // Warning. Loglevels [0, 6] 0 ... nothing, 6 ... trace
    int Config::verboseLevel = 0;

    std::shared_ptr<CLI::Formatter> Config::sectionFormatter = std::make_shared<CLI::Formatter>();

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

        logger::Logger::init();

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

        sectionFormatter->label("SUBCOMMAND", "SECTION");
        sectionFormatter->label("SUBCOMMANDS", "SECTIONS");
        sectionFormatter->column_width(40);

        app.get_formatter()->label("SUBCOMMAND", "INSTANCE");
        app.get_formatter()->label("SUBCOMMANDS", "INSTANCES");
        app.get_formatter()->column_width(40);

        app.description("Configuration for Application " + applicationName);
        app.allow_extras();
        app.allow_config_extras();

        app.set_help_flag();

        app.add_flag_callback(
               "-h,--help",
               []() {
                   throw CLI::CallForHelp();
               },
               "Print this help message and exit")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        app.add_flag_callback(
               "--help-all",
               []() {
                   throw CLI::CallForAllHelp();
               },
               "Expand all help")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        app.add_flag("-d,!-f,--daemonize,!--foreground", "Start application as daemon");

        app.add_flag("-k,--kill", "Kill running daemon")->disable_flag_override()->configurable(false);

        app.add_flag("-s,--show-config", "Show current configuration and exit")->disable_flag_override()->configurable(false);

        app.add_flag("-w{" + defaultConfDir + "/" + applicationName + ".conf" + "},--write-config{" + defaultConfDir + "/" +
                         applicationName + ".conf" + "}",
                     outputConfigFile,
                     "Write config file")
            ->configurable(false);

        app.add_flag("-l{" + defaultLogDir + "/" + applicationName + ".log" + "},--log-file{" + defaultLogDir + "/" + applicationName +
                         ".log" + "}",
                     logFile,
                     "Logfile path")
            ->default_val(defaultLogDir + "/" + applicationName + ".log")
            ->type_name("");

        app.add_flag("-e,--enforce-log-file", "Enforce writing of logs to file for foreground applications")->default_val("false");

        app.add_option("--log-level", logLevel, "Log level")->default_val(3)->type_name("([0-6])");

        app.add_option("--verbose-level", verboseLevel, "Verbosity level")->default_val(0)->type_name("([0-9]|10)");

        app.set_config("-c,--config", defaultConfDir + "/" + applicationName + ".conf", "Read an config file", false);

        app.footer("Application powered by SNode.C (C) 2019-2023 Volker Christian\n"
                   "https://github.com/VolkerChristian/snode.c - me@vchrist.at");

        bool ret = parse(false); // for stopDaemon but do not act on -h or --help-all

        if (app["--kill"]->count() > 0) {
            utils::Daemon::stopDaemon(defaultPidDir + "/" + applicationName + ".pid");

            ret = false;
        } else {
            logger::Logger::setLogLevel(logLevel);
            logger::Logger::setVerboseLevel(verboseLevel);
        }

        return ret;
    }

    bool Config::prepare() {
        bool ret = parse(app["--show-config"]->count() == 0);

        if (ret) {
            if (app["--show-config"]->count() > 0) {
                VLOG(0) << "Show current configuration\n" << app.config_to_str(true, true);

                ret = false;
            } else if (app["--write-config"]->count() > 0) {
                VLOG(0) << "Write config file " << outputConfigFile;
                std::ofstream confFile(outputConfigFile);
                confFile << app.config_to_str(true, true);

                ret = false;
            } else if (app["--daemonize"]->as<bool>()) {
                VLOG(0) << "Running as daemon";

                ret = utils::Daemon::startDaemon(defaultPidDir + "/" + applicationName + ".pid");

                if (ret) {
                    logger::Logger::quiet();
                    if (!logFile.empty()) {
                        logger::Logger::logToFile(logFile);
                    }
                } else {
                    VLOG(0) << "Daemon already running: Not daemonized ... exiting";
                }
            } else {
                VLOG(0) << "Running in foureground";

                if (app["--enforce-log-file"]->as<bool>()) {
                    VLOG(0) << "Writing logs to file " << logFile;

                    logger::Logger::logToFile(logFile);
                }
            }
        }

        return ret;
    }

    void Config::terminate() {
        if (app["--daemonize"]->as<bool>()) {
            Daemon::erasePidFile(defaultPidDir + "/" + applicationName + ".pid");
        }

        if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) >= 0) {
            char buf[1024];
            while (read(STDIN_FILENO, buf, 1024) > 0)
                ;
        }
    }

    CLI::App* Config::add_instance(const std::string& name, const std::string& description) {
        app.require_subcommand(0, app.get_require_subcommand_max() + 1);

        CLI::App* instance = app.add_subcommand(name, description)->formatter(sectionFormatter);

        instance
            ->add_flag_callback(
                "-h,--help",
                []() {
                    throw CLI::CallForHelp();
                },
                "Print this help message and exit")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        instance
            ->add_flag_callback(
                "--help-all",
                []() {
                    throw CLI::CallForAllHelp();
                },
                "Expand all help")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        return instance;
    }

    bool Config::remove_instance(CLI::App* instance) {
        return app.remove_subcommand(instance);
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

    int Config::getLogLevel() {
        return logLevel;
    }

    int Config::getVerboseLevel() {
        return verboseLevel;
    }

    bool Config::parse(bool stopOnError) {
        bool ret = true;

        Config::app.allow_extras(!stopOnError);

        try {
            Config::app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            if (stopOnError) {
                if (e.get_name() != "CallForHelp" && e.get_name() != "CallForAllHelp") {
                    std::cout << "Command line error: " << e.what() << std::endl << std::endl;
                    std::cout << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
                } else {
                    Config::app.exit(e);
                }
                ret = false;
            }
        }

        return ret;
    }

} // namespace utils
