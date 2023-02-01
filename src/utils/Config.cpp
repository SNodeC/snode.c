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
#include "utils/ResetToDefault.h"

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
#include <vector>

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

    std::string Config::defaultConfDir;
    std::string Config::defaultLogDir;
    std::string Config::defaultPidDir;

    CLI::Option* Config::daemonizeOpt = nullptr;
    CLI::Option* Config::logFileOpt = nullptr;
    CLI::Option* Config::enforceLogFileOpt = nullptr;
    CLI::Option* Config::logLevelOpt = nullptr;
    CLI::Option* Config::verboseLevelOpt = nullptr;

    std::shared_ptr<CLI::Formatter> Config::sectionFormatter = std::make_shared<CLI::Formatter>();

    class CallForCommandline : public CLI::Error {
    public:
        CallForCommandline(CLI::App* app, bool defaultsAlso)
            : CLI::Error("CallForCommandline", "Call for a template command line is requested.")
            , app(app)
            , defaultsAlso(defaultsAlso) {
        }

        CLI::App* getApp() const {
            return app;
        }

        bool getDefaultsAlso() const {
            return defaultsAlso;
        }

    private:
        CLI::App* app;
        bool defaultsAlso;
    };

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
        sectionFormatter->column_width(53);

        app.configurable(false);
        app.allow_extras();
        app.allow_config_extras();
        app.set_help_flag();

        app.get_formatter()->label("SUBCOMMAND", "INSTANCE");
        app.get_formatter()->label("SUBCOMMANDS", "INSTANCES");
        app.get_formatter()->column_width(53);

        app.get_config_formatter_base()->commentDefaults();

        app.description("#################################################################\n\n"
                        "Configuration for Application '" +
                        applicationName +
                        "'\n"
                        "Options with default values are commented in the config file\n\n"
                        "#################################################################");

        app.footer("Application powered by SNode.C (C) 2019-2023 Volker Christian\n"
                   "https://github.com/VolkerChristian/snode.c - me@vchrist.at");

        app.add_flag_callback(
               "-h,--help",
               []() {
                   throw CLI::CallForHelp();
               },
               "Print this help message and exit") //
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        app.add_flag_callback(
               "--help-all",
               []() {
                   throw CLI::CallForAllHelp();
               },
               "Expand all help") //
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        app.add_flag_callback(
               "--commandline",
               []() {
                   throw CallForCommandline(&app, false);
               },
               "Print a template command line showing required options only and exit") //
            ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
            ->configurable(false)
            ->disable_flag_override();

        app.add_flag_callback(
               "--commandline-long",
               []() {
                   throw CallForCommandline(&app, true);
               },
               "Print a template command line showing all possible options and exit") //
            ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
            ->configurable(false)
            ->disable_flag_override();

        daemonizeOpt = app.add_flag_function("-d,!-f,--daemonize,!--foreground",
                                             utils::ResetToDefault(daemonizeOpt),
                                             "Start application as daemon") //
                           ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
                           ->default_val("false")
                           ->type_name("bool")
                           ->check(CLI::TypeValidator<bool>())
                           ->force_callback();

        app.add_flag("-k,--kill", "Kill running daemon") //
            ->configurable(false)
            ->disable_flag_override();

        app.add_flag("-s,--show-config", "Show current configuration and exit") //
            ->configurable(false)
            ->disable_flag_override();

        app.add_option("-w,--write-config", "Write config file and exit") //
            ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
            ->configurable(false)
            ->default_val(defaultConfDir + "/" + applicationName + ".conf")
            ->type_name("[configfile]")
            ->check(!CLI::ExistingDirectory)
            ->expected(0, 1);

        logFileOpt = app.add_option_function<std::string>("-l,--log-file",
                                                          utils::ResetToDefault(logFileOpt),
                                                          "Logfile path") //
                         ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
                         ->default_val(defaultLogDir + "/" + applicationName + ".log")
                         ->type_name("logfile")
                         ->check(!CLI::ExistingDirectory)
                         ->force_callback();

        enforceLogFileOpt = app.add_flag_function("-e,--enforce-log-file",
                                                  utils::ResetToDefault(enforceLogFileOpt),
                                                  "Enforce writing of logs to file for foreground applications") //
                                ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
                                ->default_val("false")
                                ->type_name("bool")
                                ->check(CLI::TypeValidator<bool>())
                                ->force_callback();

        logLevelOpt = app.add_option_function<std::string>("--log-level",
                                                           utils::ResetToDefault(logLevelOpt),
                                                           "Log level") //
                          ->default_val(3)
                          ->type_name("level")
                          ->check(CLI::Range(0, 6))
                          ->force_callback();

        verboseLevelOpt = app.add_option_function<std::string>("--verbose-level", utils::ResetToDefault(logFileOpt), "Verbose level") //
                              ->default_val(0)
                              ->type_name("level")
                              ->check(CLI::Range(0, 10))
                              ->force_callback();

        app.set_version_flag("--version", "1.0.0");

        app.set_config("-c,--config", defaultConfDir + "/" + applicationName + ".conf", "Read an config file", false);

        bool ret = true;

        ret = parse(false); // for stopDaemon but do not act on -h or --help-all

        if (app["--kill"]->count() > 0) {
            utils::Daemon::stopDaemon(defaultPidDir + "/" + applicationName + ".pid");

            ret = false;
        } else {
            logger::Logger::setLogLevel(logLevelOpt->as<int>());
            logger::Logger::setVerboseLevel(verboseLevelOpt->as<int>());
        }

        return ret;
    }

    bool Config::prepare() {
        bool ret = parse(app["--show-config"]->count() == 0 && app["--write-config"]->count() == 0);

        if (ret) {
            if (app["--show-config"]->count() > 0) {
                try {
                    VLOG(0) << "Show current configuration\n" << app.config_to_str(true, true);
                } catch (CLI::ValidationError& e) {
                    std::cout << "Command line or config file error: " << e.what() << std::endl << std::endl;
                    std::cout << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
                }
                ret = false;
            } else if (app["--write-config"]->count() > 0) {
                VLOG(0) << "Write config file " << app["--write-config"]->as<std::string>();
                std::ofstream confFile(app["--write-config"]->as<std::string>());
                try {
                    confFile << app.config_to_str(true, true);
                } catch (CLI::ValidationError& e) {
                    std::cout << "Command lineor config file error: " << e.what() << std::endl << std::endl;
                    std::cout << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
                }
                ret = false;
            } else if (app["--daemonize"]->as<bool>()) {
                VLOG(0) << "Running as daemon";

                ret = utils::Daemon::startDaemon(defaultPidDir + "/" + applicationName + ".pid");

                if (ret) {
                    logger::Logger::quiet();

                    std::string logFile = logFileOpt->as<std::string>();
                    if (!logFile.empty()) {
                        logger::Logger::logToFile(logFile);
                    }
                } else {
                    VLOG(0) << "Daemon already running: Not daemonized ... exiting";
                }
            } else {
                if (app["--enforce-log-file"]->as<bool>()) {
                    std::string logFile = logFileOpt->as<std::string>();
                    VLOG(0) << "Writing logs to file " << logFile;

                    logger::Logger::logToFile(logFile);
                }
            }
        }

        return ret;
    }

    void createCommandLineOptions(std::stringstream& optionOut, CLI::App* app, bool defaultsAlso);
    void createCommandLineOptions(std::stringstream& optionOut, CLI::App* app, bool defaultsAlso) {
        std::vector<CLI::Option*> appOptions = app->get_options();

        if (!appOptions.empty()) {
            for (const CLI::Option* option : appOptions) {
                if (option->get_configurable()) {
                    std::string value;

                    if (option->reduced_results().size() > 0 && (option->get_required() || defaultsAlso)) {
                        value = option->reduced_results()[0];
                    } else if (!option->get_default_str().empty()) {
                        value = defaultsAlso ? option->get_default_str() : "";
                    } else {
                        if (option->get_required()) {
                            value = "<REQUIRED>";
                        } else {
                            value = defaultsAlso ? "\"\"" : "";
                        }
                    }

                    if (!value.empty()) {
                        if (option->get_expected_min() == 0) {
                            optionOut << "--" << option->get_single_name() << "=" << value << " ";
                        } else {
                            optionOut << "--" << option->get_single_name() << " " << value << " ";
                        }
                    }
                }
            }
        }
    }

    std::string createCommandLineOptions(CLI::App* app, bool defaultsAlso);
    std::string createCommandLineOptions(CLI::App* app, bool defaultsAlso) {
        std::stringstream optionOut;

        createCommandLineOptions(optionOut, app, defaultsAlso);

        std::string optionString = optionOut.str();

        if (optionString.back() == ' ') {
            optionString.pop_back();
        }

        return optionString;
    }

    void createCommandLineTemplate(std::stringstream& out, CLI::App* app, bool defaultsAlso);
    std::string createCommandLineSubcommands(CLI::App* app, bool defaultsAlso);
    std::string createCommandLineSubcommands(CLI::App* app, bool defaultsAlso) {
        std::stringstream subcommandOut;

        std::vector<CLI::App*> appSubcommands = app->get_subcommands({});
        if (!appSubcommands.empty()) {
            for (CLI::App* subcommand : appSubcommands) {
                createCommandLineTemplate(subcommandOut, subcommand, defaultsAlso);
            }
        }

        return subcommandOut.str();
    }

    void createCommandLineTemplate(std::stringstream& out, CLI::App* app, bool defaultsAlso) {
        std::string outString;

        outString = createCommandLineOptions(app, defaultsAlso);
        if (!outString.empty()) {
            outString += " ";
        }
        outString += createCommandLineSubcommands(app, defaultsAlso);

        if (!outString.empty()) {
            out << app->get_name() << " " << outString;
        }
    }

    std::string createCommandLineTemplate(CLI::App* app, bool defaultsAlso);
    std::string createCommandLineTemplate(CLI::App* app, bool defaultsAlso) {
        std::stringstream out;

        createCommandLineTemplate(out, app, defaultsAlso);

        std::string outString = out.str();

        while (app->get_parent() != nullptr) {
            app = app->get_parent();
            outString = app->get_name() + " " + createCommandLineOptions(app, false) + outString;
        }

        return outString;
    }

    bool Config::parse(bool stopOnError) {
        bool ret = true;

        Config::app.allow_extras(!stopOnError)->allow_config_extras(!stopOnError);

        try {
            Config::app.parse(argc, argv);
        } catch (const CallForCommandline& e) {
            if (stopOnError) {
                if (e.getDefaultsAlso()) {
                    std::cout << "Below is a full template command line" << std::endl;
                    std::cout << "* Required but not yet configured options show the value <REQUIRED>" << std::endl;
                    std::cout << "* All other options showing either their default or current configured value" << std::endl << std::endl;
                } else {
                    std::cout << "Below is a minimal template command line" << std::endl;
                    std::cout << "* Only required options are showen" << std::endl;
                    std::cout << "* Required but not yet configured options show the value <REQUIRED>" << std::endl;
                    std::cout << "* All other options show their current configured value" << std::endl << std::endl;
                }
                std::cout << createCommandLineTemplate(e.getApp(), e.getDefaultsAlso()) << std::endl;
                ret = false;
            }
        } catch (const CLI::ConversionError& e) {
            std::cout << "Command line or config file error: " << e.what() << std::endl << std::endl;
            std::cout << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
            ret = false;
        } catch (const CLI::ConfigError& e) {
            if (stopOnError) {
                std::cout << "Command line or config file error: " << e.what() << std::endl;
                std::cout << "Rewrite the config file by appending -w [config file] to your command line." << std::endl << std::endl;
                std::cout << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
                ret = false;
            }
        } catch (const CLI::ParseError& e) {
            if (stopOnError) {
                if (e.get_name() != "CallForHelp" && e.get_name() != "CallForAllHelp" && e.get_name() != "CallForVersion") {
                    std::cout << "Command line or config file error: " << e.what() << std::endl << std::endl;
                    std::cout << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
                } else {
                    Config::app.exit(e);
                }
                ret = false;
            }
        }

        return ret;
    } // namespace utils

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
        CLI::App* instance = app.add_subcommand(name, description) //
                                 ->configurable(false)
                                 ->allow_extras(false)
                                 ->formatter(sectionFormatter)
                                 ->fallthrough()
                                 ->group(name.empty() ? "" : "Instances")
                                 ->disabled(name.empty())
                                 ->silent(name.empty());

        instance //
            ->option_defaults()
            ->configurable(!name.empty())
            ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);

        instance //
            ->add_flag_callback(
                "-h,--help",
                []() {
                    throw CLI::CallForHelp();
                },
                "Print this help message and exit") //
            ->configurable(false)                   //
            ->disable_flag_override()               //
            ->trigger_on_parse();

        instance //
            ->add_flag_callback(
                "--help-all",
                []() {
                    throw CLI::CallForAllHelp();
                },
                "Expand all help")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        instance //
            ->add_flag_callback(
                "--commandline",
                [instance]() {
                    throw CallForCommandline(instance, false);
                },
                "Print a template command line showing required options only and exit")
            ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
            ->configurable(false)
            ->disable_flag_override();

        instance //
            ->add_flag_callback(
                "--commandline-long",
                [instance]() {
                    throw CallForCommandline(instance, true);
                },
                "Print a template command line showing all possible options and exit")
            ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
            ->configurable(false)
            ->disable_flag_override();

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
        add_option(name, variable, description) //
            ->required(required)                //
            ->default_val(default_val)          //
            ->type_name(typeName)               //
            ->configurable(configurable)        //
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
        add_option(name, variable, description) //
            ->required(required)                //
            ->default_val(default_val)          //
            ->type_name(typeName)               //
            ->configurable(configurable)        //
            ->group(groupName);
    }

    void Config::add_flag(const std::string& name,
                          bool& variable,
                          const std::string& description,
                          bool required,
                          bool configurable,
                          const std::string& groupName) {
        add_flag(name, variable, description) //
            ->required(required)              //
            ->configurable(configurable)      //
            ->group(groupName);
    }

    CLI::Option* Config::add_option(const std::string& name, int& variable, const std::string& description) {
        return app.add_option(name, variable, description)->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
    }

    CLI::Option* Config::add_option(const std::string& name, std::string& variable, const std::string& description) {
        return app.add_option(name, variable, description)->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
    }

    CLI::Option* Config::add_flag(const std::string& name, const std::string& description) {
        return app.add_flag(name, description)->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
    }

    CLI::Option* Config::add_flag(const std::string& name, bool& variable, const std::string& description) {
        return app.add_flag(name, variable, description)->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
    }

    std::string Config::getApplicationName() {
        return applicationName;
    }

    int Config::getLogLevel() {
        return logLevelOpt->as<int>();
    }

    int Config::getVerboseLevel() {
        return verboseLevelOpt->as<int>();
    }

} // namespace utils
