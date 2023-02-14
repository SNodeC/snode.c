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

namespace CLI {

    class CallForCommandline : public CLI::Success {
    public:
        enum class Mode { SHORT, MEDIUM, LONG };

        CallForCommandline(CLI::App* app, Mode mode)
            : CLI::Success("CallForCommandline", "A template command line is showen below:\n", CLI::ExitCodes::Success)
            , app(app)
            , mode(mode) {
        }

        CLI::App* getApp() const {
            return app;
        }

        Mode getMode() const {
            return mode;
        }

    private:
        CLI::App* app;
        Mode mode;
    };

    class CallForShowConfig : public CLI::Success {
    public:
        CallForShowConfig()
            : CLI::Success("CallForPrintConfig", "Show current configuration", CLI::ExitCodes::Success) {
        }
    };

    class CallForWriteConfig : public CLI::Success {
    public:
        explicit CallForWriteConfig(const std::string& configFile)
            : CLI::Success("CallForWriteConfig", "Writing config file: " + configFile, CLI::ExitCodes::Success)
            , configFile(configFile) {
        }

        std::string getConfigFile() const {
            return configFile;
        }

    private:
        std::string configFile;
    };

    class DaemonAlreadyRunningError : public CLI::Error {
    public:
        DaemonAlreadyRunningError()
            : CLI::Error("DaemonAlreadyRunning", "Daemon already running: Not daemonized ... exiting") {
        }
    };

} // namespace CLI

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

    std::map<std::string, std::string> Config::prefixMap;

    // Declare some used functions
    static std::string createCommandLineOptions(CLI::App* app, CLI::CallForCommandline::Mode mode);
    static void createCommandLineOptions(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode);
    static std::string createCommandLineSubcommands(CLI::App* app, CLI::CallForCommandline::Mode mode);
    static void createCommandLineTemplate(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode);
    static std::string createCommandLineTemplate(CLI::App* app, CLI::CallForCommandline::Mode mode);

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

        app.footer("Application '" + applicationName +
                   "' powered by SNode.C\n"
                   "(C) 2019-2023 Volker Christian\n"
                   "https://github.com/VolkerChristian/snode.c - me@vchrist.at");

        app.option_defaults()->take_last();

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

        app.add_flag("--commandline",
                     "Print a template command line showing required options only and exit") //
            ->configurable(false)
            ->disable_flag_override();

        app.add_flag("--commandline-configured",
                     "Print a template command line showing all required and configured options and exit") //
            ->configurable(false)
            ->disable_flag_override();

        app.add_flag("--commandline-full",
                     "Print a template command line showing all possible options and exit") //
            ->configurable(false)
            ->disable_flag_override();

        app.add_flag("-s,--show-config", "Show current configuration and exit") //
            ->configurable(false)
            ->disable_flag_override();

        app.add_option("-w,--write-config", "Write config file and exit") //
            ->configurable(false)
            ->default_val(defaultConfDir + "/" + applicationName + ".conf")
            ->type_name("[configfile]")
            ->check(!CLI::ExistingDirectory)
            ->expected(0, 1);

        daemonizeOpt = app.add_flag_function("-d,!-f,--daemonize,!--foreground",
                                             utils::ResetToDefault(daemonizeOpt),
                                             "Start application as daemon") //
                           ->take_last()
                           ->default_val("false")
                           ->type_name("bool")
                           ->check(CLI::TypeValidator<bool>());

        app.add_flag("-k,--kill", "Kill running daemon") //
            ->configurable(false)
            ->disable_flag_override();

        logFileOpt = app.add_option_function<std::string>("-l,--log-file",
                                                          utils::ResetToDefault(logFileOpt),
                                                          "Logfile path") //
                         ->default_val(defaultLogDir + "/" + applicationName + ".log")
                         ->type_name("logfile")
                         ->check(!CLI::ExistingDirectory);

        enforceLogFileOpt = app.add_flag_function("-e,--enforce-log-file",
                                                  utils::ResetToDefault(enforceLogFileOpt),
                                                  "Enforce writing of logs to file for foreground applications") //
                                ->take_last()
                                ->default_val("false")
                                ->type_name("bool")
                                ->check(CLI::TypeValidator<bool>());

        logLevelOpt = app.add_option_function<std::string>("--log-level",
                                                           utils::ResetToDefault(logLevelOpt),
                                                           "Log level") //
                          ->default_val(3)
                          ->type_name("level")
                          ->check(CLI::Range(0, 6));

        verboseLevelOpt =
            app.add_option_function<std::string>("--verbose-level", utils::ResetToDefault(verboseLevelOpt), "Verbose level") //
                ->default_val(0)
                ->type_name("level")
                ->check(CLI::Range(0, 10));

        app.set_version_flag("--version", "0.9.8");

        app.set_config("-c,--config", defaultConfDir + "/" + applicationName + ".conf", "Read an config file", false);

        app.add_option("--instance-map", "Instance name mapping used to make an instance known under an alias name also in a config file.")
            ->configurable(false)
            ->type_name("name=mapped_name")
            ->each([](const std::string& item) -> void {
                const auto it = item.find('=');
                if (it != item.npos) {
                    prefixMap[item.substr(0, it)] = item.substr(it + 1);
                } else {
                    throw CLI::ConversionError("Can not convert '" + item + "' to a 'name=mapped_name' pair");
                }
            });

        bool ret = parse(); // for stopDaemon, logLevel and verboseLevel but do not act on -h or --help-all

        if (ret) {
            try {
                if (app["--kill"]->count() > 0) {
                    utils::Daemon::stopDaemon(defaultPidDir + "/" + applicationName + ".pid");

                    ret = false;
                } else {
                    logger::Logger::setLogLevel(logLevelOpt->as<int>());
                    logger::Logger::setVerboseLevel(verboseLevelOpt->as<int>());
                }
            } catch (const CLI::ParseError&) {
                // Do not error here but in the second pass
            }
        }

        app.all_config_files(!prefixMap.empty());

        return ret;
    }

    bool Config::bootstrap() {
        prefixMap.clear();

        app.final_callback([](void) -> void {
            if (app["--show-config"]->count() > 0) {
                throw CLI::CallForShowConfig();
            } else if (app["--write-config"]->count() > 0) {
                throw CLI::CallForWriteConfig(app["--write-config"]->as<std::string>());
            } else if (app["--commandline"]->count() > 0) {
                throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::SHORT);
            } else if (app["--commandline-configured"]->count() > 0) {
                throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::MEDIUM);
            } else if (app["--commandline-full"]->count() > 0) {
                throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::LONG);
            } else {
                if (app["--daemonize"]->as<bool>()) {
                    VLOG(0) << "Try Running as daemon";

                    if (utils::Daemon::startDaemon(defaultPidDir + "/" + applicationName + ".pid")) {
                        logger::Logger::quiet();

                        std::string logFile = logFileOpt->as<std::string>();
                        if (!logFile.empty()) {
                            logger::Logger::logToFile(logFile);
                        }
                    } else {
                        throw CLI::DaemonAlreadyRunningError();
                    }
                } else if (app["--enforce-log-file"]->as<bool>()) {
                    std::string logFile = logFileOpt->as<std::string>();
                    VLOG(0) << "Writing logs to file " << logFile;

                    logger::Logger::logToFile(logFile);
                }
            }
        });

        return parse(app["--show-config"]->count() == 0 && app["--write-config"]->count() == 0 && app["--commandline"]->count() == 0 &&
                     app["--commandline-configured"]->count() == 0 && app["--commandline-full"]->count() == 0);
    }

    void createCommandLineOptions(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode) {
        for (const CLI::Option* option : app->get_options()) {
            if (option->get_configurable()) {
                std::string value;

                if (option->reduced_results().size() > 0 && ((option->get_required() || mode == CLI::CallForCommandline::Mode::LONG) ||
                                                             mode == CLI::CallForCommandline::Mode::MEDIUM)) {
                    value = option->reduced_results()[0];
                } else if (!option->get_default_str().empty()) {
                    value = mode == CLI::CallForCommandline::Mode::LONG ? option->get_default_str() : "";
                } else {
                    if (option->get_required()) {
                        value = "<REQUIRED>";
                    } else {
                        value = mode == CLI::CallForCommandline::Mode::LONG ? "\"\"" : "";
                    }
                }

                if (!value.empty()) {
                    if (option->get_expected_min() == 0) {
                        out << "--" << option->get_single_name() << "=" << value << " ";
                    } else {
                        out << "--" << option->get_single_name() << " " << value << " ";
                    }
                }
            }
        }
    }

    std::string createCommandLineOptions(CLI::App* app, CLI::CallForCommandline::Mode mode) {
        std::stringstream out;

        createCommandLineOptions(out, app, mode);

        std::string optionString = out.str();
        if (optionString.back() == ' ') {
            optionString.pop_back();
        }

        return optionString;
    }

    std::string createCommandLineSubcommands(CLI::App* app, CLI::CallForCommandline::Mode mode) {
        std::stringstream out;

        for (CLI::App* subcommand : app->get_subcommands({})) {
            createCommandLineTemplate(out, subcommand, mode);
        }

        return out.str();
    }

    void createCommandLineTemplate(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode) {
        std::string outString;

        outString = createCommandLineOptions(app, mode);
        if (!outString.empty()) {
            outString += " ";
        }

        outString += createCommandLineSubcommands(app, mode);
        if (!outString.empty()) {
            out << app->get_name() << " " << outString;
        }
    }

    std::string createCommandLineTemplate(CLI::App* app, CLI::CallForCommandline::Mode mode) {
        std::stringstream out;

        createCommandLineTemplate(out, app, mode);

        std::string outString = out.str();
        while (app->get_parent() != nullptr) {
            app = app->get_parent();
            outString = app->get_name() + " " + createCommandLineOptions(app, CLI::CallForCommandline::Mode::MEDIUM) + " " + outString;
        }

        return outString;
    }

    namespace Color {

        enum class Code {
            FG_DEFAULT = 39,
            FG_BLACK = 30,
            FG_RED = 31,
            FG_GREEN = 32,
            FG_YELLOW = 33,
            FG_BLUE = 34,
            FG_MAGENTA = 35,
            FG_CYAN = 36,
            FG_LIGHT_GRAY = 37,
            FG_DARK_GRAY = 90,
            FG_LIGHT_RED = 91,
            FG_LIGHT_GREEN = 92,
            FG_LIGHT_YELLOW = 93,
            FG_LIGHT_BLUE = 94,
            FG_LIGHT_MAGENTA = 95,
            FG_LIGHT_CYAN = 96,
            FG_WHITE = 97,
            BG_RED = 41,
            BG_GREEN = 42,
            BG_BLUE = 44,
            BG_DEFAULT = 49
        };

        std::ostream& operator<<(std::ostream& os, Color::Code code);
        std::ostream& operator<<(std::ostream& os, Color::Code code) {
            return os << "\033[" << static_cast<int>(code) << "m";
        }

    } // namespace Color

    bool Config::parse() {
        bool ret = true;

        try {
            app.parse(argc, argv);
        } catch (const CLI::CallForHelp& e) {
            // Do not process --help here but on second parse pass
        } catch (const CLI::CallForAllHelp& e) {
            // Do not process --help-all here but on second parse pass
        } catch (const CLI::CallForVersion& e) {
            // Do not process --version here but on second parse pass
        } catch (const CLI::CallForCommandline& e) {
            // Do not process --commandline here but on second parse pass
        } catch (const CLI::ParseError& e) {
            std::cout << "ParseError: " << e.what() << std::endl;
            std::cout << "Hint: In case you have used a multi argument option try adding '--' after the last option argument" << std::endl;
            ret = false;
        }

        return ret;
    }

    bool Config::parse(bool stopOnError) {
        bool ret = false;

        Config::app //
            .allow_extras(!stopOnError)
            ->allow_config_extras(!stopOnError);

        try {
            try {
                try {
                    app.parse(argc, argv);
                    ret = true;
                } catch (const CLI::ParseError& e) {
                    if (!stopOnError) {
                        if (app["--show-config"]->count() > 0) {
                            throw CLI::CallForShowConfig();
                        } else if (app["--write-config"]->count() > 0) {
                            throw CLI::CallForWriteConfig(app["--write-config"]->as<std::string>());
                        } else if (app["--commandline"]->count() > 0) {
                            throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::SHORT);
                        } else if (app["--commandline-configured"]->count() > 0) {
                            throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::MEDIUM);
                        } else if (app["--commandline-full"]->count() > 0) {
                            throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::LONG);
                        } else if (app["--help"]->count() > 0) {
                            throw CLI::CallForHelp();
                        } else if (app["--help-all"]->count() > 0) {
                            throw CLI::CallForAllHelp();
                        }
                    } else {
                        throw;
                    }
                }
            } catch (const CLI::CallForHelp& e) {
                std::cout << app.help();
            } catch (const CLI::CallForAllHelp& e) {
                std::cout << app.help("", CLI::AppFormatMode::All);
            } catch (const CLI::CallForVersion& e) {
                std::cout << "SNode.C-Version: " << app.version() << std::endl;
                throw;
            } catch (const CLI::CallForCommandline& e) {
                std::cout << e.what();
                if (e.getMode() == CLI::CallForCommandline::Mode::LONG) {
                    std::cout << "* Required but not yet configured options show <REQUIRED> as value " << std::endl;
                    std::cout << "* Remaining options show either their default or configured value" << std::endl;
                    std::cout << "* Options marked as <REQUIRED> need to be configured for a successfull bootstrap" << std::endl;
                } else if (e.getMode() == CLI::CallForCommandline::Mode::SHORT) {
                    std::cout << "* Required options show eigher their configured value or <REQUIRED>" << std::endl;
                    std::cout << "* Options marked as <REQUIRED> need to be configured for a successfull bootstrap" << std::endl;
                } else {
                    std::cout << "* Required but not yet configured options show <REQUIRED> as value" << std::endl;
                    std::cout << "* All other options show their currently configured value" << std::endl;
                    std::cout << "* Options marked as <REQUIRED> need to be configured for a successfull bootstrap" << std::endl;
                }
                std::cout << std::endl
                          << Color::Code::FG_GREEN << "command@line" << Color::Code::FG_DEFAULT << ":" << Color::Code::FG_BLUE << "~/> "
                          << Color::Code::FG_DEFAULT << createCommandLineTemplate(e.getApp(), e.getMode()) << std::endl;

                std::cout << std::endl << app.get_footer() << std::endl;
            } catch (const CLI::CallForShowConfig& e) {
                std::cout << e.what() << std::endl;
                std::cout << app.config_to_str(true, true);
            } catch (const CLI::CallForWriteConfig& e) {
                std::cout << e.what() << std::endl;
                std::ofstream confFile(e.getConfigFile());
                if (confFile.is_open()) {
                    confFile << app.config_to_str(true, true);
                    confFile.close();
                }
                std::cout << std::endl << app.get_footer() << std::endl;
            } catch (const CLI::ConversionError& e) {
                std::cout << "Conversion error: " << e.what() << std::endl;
                throw;
            } catch (const CLI::ConfigError& e) {
                std::cout << "Config file (INI) parse error: " << e.get_name() << e.what() << std::endl;
                std::cout << "Hint: Rewrite the config file by appending -w [config file] to your command line." << std::endl;
                throw;
            } catch (const CLI::ParseError& e) {
                std::cout << "Command line error: " << e.what() << std::endl;
                throw;
            }
        } catch (const CLI::ParseError& e) {
            std::cout << std::endl << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
            std::cout << std::endl << app.get_footer() << std::endl;
        } catch (const CLI::Error& e) {
            std::cout << "Error: " << e.what() << std::endl;
            std::cout << std::endl << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
            std::cout << std::endl << app.get_footer() << std::endl;
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
            ->configurable(!name.empty());

        if (!prefixMap.empty()) {
            if (prefixMap.contains(name)) {
                instance //
                    ->alias(prefixMap[name]);
            } else {
                instance //
                    ->option_defaults()
                    ->take_first();
            }
        }

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
                    throw CLI::CallForCommandline(instance, CLI::CallForCommandline::Mode::SHORT);
                },
                "Print a template command line showing required options only and exit")
            ->configurable(false)
            ->disable_flag_override();

        instance //
            ->add_flag_callback(
                "--commandline-full",
                [instance]() {
                    throw CLI::CallForCommandline(instance, CLI::CallForCommandline::Mode::LONG);
                },
                "Print a template command line showing all possible options and exit")
            ->configurable(false)
            ->disable_flag_override();

        instance //
            ->add_flag_callback(
                "--commandline-configured",
                []() {
                    throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::MEDIUM);
                },
                "Print a template command line showing all required and configured options and exit") //
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
        return logLevelOpt->as<int>();
    }

    int Config::getVerboseLevel() {
        return verboseLevelOpt->as<int>();
    }

} // namespace utils
