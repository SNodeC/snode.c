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

#include "utils/Daemon.h"
#include "utils/Formatter.h"
#include "utils/ResetToDefault.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "log/Logger.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <grp.h>
#include <iostream>
#include <memory>
#include <pwd.h>
#include <stdexcept>
#include <unistd.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define XSTR(s) STR(s)
#define STR(s) #s

namespace CLI {

    class CallForCommandline : public CLI::Success {
    public:
        enum class Mode { REQUIRED, CONFIGURED, ALL };

        CallForCommandline(CLI::App* app, Mode mode)
            : CLI::Success("CallForCommandline", "A template command line is showen below:\n", CLI::ExitCodes::Success)
            , app(app)
            , mode(mode) {
        }

        ~CallForCommandline() override;

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

    CallForCommandline::~CallForCommandline() {
    }

    class CallForShowConfig : public CLI::Success {
    public:
        CallForShowConfig()
            : CLI::Success("CallForPrintConfig", "Show current configuration", CLI::ExitCodes::Success) {
        }

        ~CallForShowConfig() override;
    };

    CallForShowConfig::~CallForShowConfig() {
    }

    class CallForWriteConfig : public CLI::Success {
    public:
        explicit CallForWriteConfig(const std::string& configFile)
            : CLI::Success("CallForWriteConfig", "Writing config file: " + configFile, CLI::ExitCodes::Success)
            , configFile(configFile) {
        }

        ~CallForWriteConfig() override;

        std::string getConfigFile() const {
            return configFile;
        }

    private:
        std::string configFile;
    };

    CallForWriteConfig::~CallForWriteConfig() {
    }

} // namespace CLI

namespace utils {

    int Config::argc = 0;
    char** Config::argv = nullptr;
    CLI::App Config::app;

    std::string Config::applicationName;

    std::string Config::configDirectory;
    std::string Config::logDirectory;
    std::string Config::pidDirectory;

    CLI::Option* Config::daemonizeOpt = nullptr;
    CLI::Option* Config::logFileOpt = nullptr;
    CLI::Option* Config::userNameOpt = nullptr;
    CLI::Option* Config::groupNameOpt = nullptr;
    CLI::Option* Config::enforceLogFileOpt = nullptr;
    CLI::Option* Config::logLevelOpt = nullptr;
    CLI::Option* Config::verboseLevelOpt = nullptr;
    CLI::Option* Config::quietOpt = nullptr;

    std::shared_ptr<CLI::Formatter> Config::sectionFormatter;
    std::map<std::string, std::string> Config::aliases;
    std::map<std::string, CLI::Option*> Config::applicationOptions;

    // Declare some used functions
    static std::string createCommandLineOptions(CLI::App* app, CLI::CallForCommandline::Mode mode);
    static void createCommandLineOptions(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode);
    static std::string createCommandLineSubcommands(CLI::App* app, CLI::CallForCommandline::Mode mode);
    static void createCommandLineTemplate(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode);
    static std::string createCommandLineTemplate(CLI::App* app, CLI::CallForCommandline::Mode mode);

    bool Config::init(int argc, char* argv[]) {
        bool success = true;

        Config::argc = argc;
        Config::argv = argv;

        logger::Logger::init();

        uid_t euid = 0;
        struct passwd* pw = nullptr;
        struct group* gr = nullptr;

        if ((pw = getpwuid(getuid())) == nullptr) {
            success = false;
        } else if ((gr = getgrgid(pw->pw_gid)) == nullptr) {
            success = false;
        } else if ((euid = geteuid()) == 0) {
            configDirectory = "/etc/snode.c";
            logDirectory = "/var/log/snode.c";
            pidDirectory = "/var/run/snode.c";
        } else {
            const char* homedir = nullptr;
            if ((homedir = std::getenv("XDG_CONFIG_HOME")) == nullptr) {
                if ((homedir = std::getenv("HOME")) == nullptr) {
                    homedir = pw->pw_dir;
                }
            }

            if (homedir != nullptr) {
                configDirectory = std::string(homedir) + "/.config/snode.c";
                logDirectory = std::string(homedir) + "/.local/log/snode.c";
                pidDirectory = std::string(homedir) + "/.local/run/snode.c";
            } else {
                success = false;
            }
        }

        applicationName = std::filesystem::path(argv[0]).filename();

        if (success && !std::filesystem::exists(configDirectory)) {
            if (std::filesystem::create_directories(configDirectory)) {
                std::filesystem::permissions(
                    configDirectory,
                    (std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec) &
                        ~std::filesystem::perms::others_all);
                if (geteuid() == 0) {
                    struct group* gr = nullptr; // cppcheck-suppress shadowVariable
                    if ((gr = getgrnam(XSTR(GROUP_NAME))) != nullptr) {
                        if (chown(configDirectory.c_str(), euid, gr->gr_gid) < 0) {
                            std::cout << "Warning: Can not set group ownership of '" << configDirectory
                                      << "' to 'snodec':" << strerror(errno) << std::endl;
                        }
                    } else {
                        std::cout << "Error: Can not find group 'snodec'. Add it using groupadd or addgroup" << std::endl;
                        std::cout << "       and add the current user to this group." << std::endl;
                        std::filesystem::remove(configDirectory);
                        success = false;
                    }
                }
            } else {
                std::cout << "Error: Can not create directory '" << configDirectory << "'" << std::endl;
                success = false;
            }
        }

        if (success && !std::filesystem::exists(logDirectory)) {
            if (std::filesystem::create_directories(logDirectory)) {
                std::filesystem::permissions(logDirectory,
                                             (std::filesystem::perms::owner_all | std::filesystem::perms::group_all) &
                                                 ~std::filesystem::perms::others_all);
                if (geteuid() == 0) {
                    struct group* gr = nullptr; // cppcheck-suppress shadowVariable
                    if ((gr = getgrnam(XSTR(GROUP_NAME))) != nullptr) {
                        if (chown(logDirectory.c_str(), euid, gr->gr_gid) < 0) {
                            std::cout << "Warning: Can not set group ownership of '" << logDirectory << "' to 'snodec':" << strerror(errno)
                                      << std::endl;
                        }
                    } else {
                        std::cout << "Error: Can not find group 'snodec'. Add it using groupadd or addgroup" << std::endl;
                        std::cout << "       and add the current user to this group." << std::endl;
                        std::filesystem::remove(configDirectory);
                        success = false;
                    }
                }
            } else {
                std::cout << "Error: Can not create directory '" << logDirectory << "'" << std::endl;
                success = false;
            }
        }

        if (success && !std::filesystem::exists(pidDirectory)) {
            if (std::filesystem::create_directories(pidDirectory)) {
                std::filesystem::permissions(pidDirectory,
                                             (std::filesystem::perms::owner_all | std::filesystem::perms::group_all) &
                                                 ~std::filesystem::perms::others_all);
                if (geteuid() == 0) {
                    struct group* gr = nullptr; // cppcheck-suppress shadowVariable
                    if ((gr = getgrnam(XSTR(GROUP_NAME))) != nullptr) {
                        if (chown(pidDirectory.c_str(), euid, gr->gr_gid) < 0) {
                            std::cout << "Warning: Can not set group ownership of '" << pidDirectory << "' to 'snodec':" << strerror(errno)
                                      << std::endl;
                        }
                    } else {
                        std::cout << "Error: Can not find group 'snodec'. Add it using groupadd or addgroup." << std::endl;
                        std::cout << "       and add the current user to this group." << std::endl;
                        std::filesystem::remove(configDirectory);
                        success = false;
                    }
                }
            } else {
                std::cout << "Error: Can not create directory '" << pidDirectory << "'" << std::endl;
                success = false;
            }
        }

        if (success) {
            sectionFormatter = std::make_shared<CLI::HelpFormatter>();

            sectionFormatter->label("SUBCOMMAND", "SECTION");
            sectionFormatter->label("SUBCOMMANDS", "SECTIONS");
            sectionFormatter->label("PERSISTENT", "");
            sectionFormatter->label("Persistent Options", "Options (persistent)");
            sectionFormatter->label("None Persistent Options", "Options (none persistent)");
            sectionFormatter->label("Usage", "\nUsage");
            sectionFormatter->label("bool:{true,false}", "{true,false}");
            sectionFormatter->column_width(7);

            app.configurable(false);
            app.set_help_flag();
            app.allow_config_extras();

            app.formatter(std::make_shared<CLI::HelpFormatter>());
            app.get_formatter()->label("SUBCOMMAND", "INSTANCE");
            app.get_formatter()->label("SUBCOMMANDS", "INSTANCES");
            app.get_formatter()->label("PERSISTENT", "");
            app.get_formatter()->label("Persistent Options", "Options (persistent)");
            app.get_formatter()->label("None Persistent Options", "Options (none persistent)");
            app.get_formatter()->label("Usage", "\nUsage");
            app.get_formatter()->label("bool:{true,false}", "{true,false}");
            app.get_formatter()->column_width(7);

            app.option_defaults()->take_last();
            app.option_defaults()->group(app.get_formatter()->get_label("None Persistent Options"));

            const std::shared_ptr<CLI::ConfigFormatter> configFormatter = std::make_shared<CLI::ConfigFormatter>();
            app.config_formatter(std::static_pointer_cast<CLI::Config>(configFormatter));

            app.description("Configuration for Application '" + applicationName + "'");

            app.footer("Application '" + applicationName +
                       "' powered by SNode.C\n"
                       "(C) 2019-2023 Volker Christian\n"
                       "https://github.com/VolkerChristian/snode.c - me@vchrist.at");

            app.set_config( //
                   "-c,--config-file",
                   configDirectory + "/" + applicationName + ".conf",
                   "Read a config file",
                   false) //
                ->take_all()
                ->type_name("configfile")
                ->check(!CLI::ExistingDirectory);

            app.add_option("-w,--write-config",
                           "Write config file and exit") //
                ->configurable(false)
                ->default_val(configDirectory + "/" + applicationName + ".conf")
                ->type_name("[configfile]")
                ->check(!CLI::ExistingDirectory)
                ->expected(0, 1);

            app.add_flag( //
                   "-s,--show-config",
                   "Show current configuration and exit") //
                ->configurable(false)
                ->disable_flag_override();

            app.add_flag_callback(
                   "--commandline",
                   []() {
                       throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::REQUIRED);
                   },
                   "Print a template command line showing required options only")
                ->configurable(false)
                ->disable_flag_override();

            app.add_flag_callback(
                   "--commandline-full",
                   []() {
                       throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::ALL);
                   },
                   "Print a template command line showing all possible options")
                ->configurable(false)
                ->disable_flag_override();

            app.add_flag_callback(
                   "--commandline-configured",
                   []() {
                       throw CLI::CallForCommandline(&app, CLI::CallForCommandline::Mode::CONFIGURED);
                   },
                   "Print a template command line showing all required and configured options") //
                ->configurable(false)
                ->disable_flag_override();

            app.add_flag( //
                   "-k,--kill",
                   "Kill running daemon") //
                ->configurable(false)
                ->disable_flag_override();

            app.add_option( //
                   "-i,--instance-alias",
                   "Make an instance also known as an alias in configuration files")
                ->configurable(false)
                ->type_name("instance=instance_alias [instance=instance_alias [...]]")
                ->each([](const std::string& item) -> void {
                    const auto it = item.find('=');
                    if (it != item.npos) {
                        aliases[item.substr(0, it)] = item.substr(it + 1);
                    } else {
                        throw CLI::ConversionError("Can not convert '" + item + "' to a 'instance=instance_alias' pair");
                    }
                });

            app.set_version_flag( //
                "--version",
                "0.9.8");

            logLevelOpt = app.add_option_function<std::string>( //
                                 "-l,--log-level",
                                 utils::ResetToDefault(logLevelOpt),
                                 "Log level") //
                              ->default_val(4)
                              ->type_name("level")
                              ->check(CLI::Range(0, 6))
                              ->group(app.get_formatter()->get_label("Persistent Options"));

            verboseLevelOpt = app.add_option_function<std::string>( //
                                     "-v,--verbose-level",
                                     utils::ResetToDefault(verboseLevelOpt),
                                     "Verbose level") //
                                  ->default_val(1)
                                  ->type_name("level")
                                  ->check(CLI::Range(0, 10))
                                  ->group(app.get_formatter()->get_label("Persistent Options"));

            quietOpt = app.add_flag_function( //
                              "-q{true},!-u,--quiet",
                              utils::ResetToDefault(quietOpt),
                              "Quiet mode") //
                           ->take_last()
                           ->default_val("false")
                           ->type_name("bool")
                           ->check(CLI::IsMember({"true", "false"}))
                           ->group(app.get_formatter()->get_label("Persistent Options"));

            logFileOpt = app.add_option_function<std::string>( //
                                "--log-file",
                                utils::ResetToDefault(logFileOpt),
                                "Logfile path") //
                             ->default_val(logDirectory + "/" + applicationName + ".log")
                             ->type_name("logfile")
                             ->check(!CLI::ExistingDirectory)
                             ->group(app.get_formatter()->get_label("Persistent Options"));

            enforceLogFileOpt = app.add_flag_function( //
                                       "-e{true},!-n,--enforce-log-file",
                                       utils::ResetToDefault(enforceLogFileOpt),
                                       "Enforce writing of logs to file for foreground applications") //
                                    ->take_last()
                                    ->default_val("false")
                                    ->type_name("bool")
                                    ->check(CLI::IsMember({"true", "false"}))
                                    ->group(app.get_formatter()->get_label("Persistent Options"));

            daemonizeOpt = app.add_flag_function( //
                                  "-d{true},!-f,--daemonize",
                                  utils::ResetToDefault(daemonizeOpt),
                                  "Start application as daemon") //
                               ->take_last()
                               ->default_val("false")
                               ->type_name("bool")
                               ->check(CLI::IsMember({"true", "false"}))
                               ->group(app.get_formatter()->get_label("Persistent Options"));

            userNameOpt = app.add_option_function<std::string>( //
                                 "--user-name",
                                 utils::ResetToDefault(userNameOpt),
                                 "Run daemon under specific user permissions") //
                              ->default_val(pw->pw_name)
                              ->type_name("username")
                              ->needs(daemonizeOpt)
                              ->group(app.get_formatter()->get_label("Persistent Options"));

            groupNameOpt = app.add_option_function<std::string>( //
                                  "--group-name",
                                  utils::ResetToDefault(groupNameOpt),
                                  "Run daemon under specific group permissions")
                               ->default_val(gr->gr_name)
                               ->type_name("groupname")
                               ->needs(daemonizeOpt)
                               ->group(app.get_formatter()->get_label("Persistent Options"));

            parse1(); // for stopDaemon and pre init application options

            if (app["--kill"]->count() > 0) {
                try {
                    utils::Daemon::stopDaemon(pidDirectory + "/" + applicationName + ".pid");
                } catch (const DaemonizeError& e) {
                    std::cout << "Daemonize error: " << e.what() << std::endl;
                    std::exit(EXIT_FAILURE);
                } catch (const DaemonizeFailure& e) {
                    std::cout << "Daemonize failure: " << e.what() << std::endl;
                    std::exit(EXIT_FAILURE);
                } catch (const DaemonizeSuccess&) {
                    std::cout << "Daemon stopped" << std::endl;
                    std::exit(EXIT_SUCCESS);
                }
            }
        }

        return success;
    }

    bool Config::bootstrap() {
        aliases.clear();

        app.final_callback([]() -> void {
            if (daemonizeOpt->as<bool>() && app["--show-config"]->count() == 0 && app["--write-config"]->count() == 0 &&
                app["--commandline"]->count() == 0 && app["--commandline-configured"]->count() == 0 &&
                app["--commandline-full"]->count() == 0) {
                std::cout << "Running as daemon" << std::endl;

                try {
                    utils::Daemon::startDaemon(
                        pidDirectory + "/" + applicationName + ".pid", userNameOpt->as<std::string>(), groupNameOpt->as<std::string>());
                } catch (const DaemonizeError& e) {
                    std::cout << "Daemonize error: " << e.what() << " ... exiting" << std::endl;
                    std::exit(EXIT_FAILURE);
                } catch (const DaemonizeFailure& e) {
                    std::cout << "Daemonize failure: " << e.what() << " ... exiting" << std::endl;
                    std::exit(EXIT_FAILURE);
                } catch (const DaemonizeSuccess&) {
                    std::exit(EXIT_SUCCESS);
                }

                logger::Logger::quiet();

                const std::string logFile = logFileOpt->as<std::string>();
                if (!logFile.empty()) {
                    logger::Logger::logToFile(logFile);
                }
            } else if (app["--enforce-log-file"]->as<bool>()) {
                const std::string logFile = logFileOpt->as<std::string>();
                std::cout << "Writing logs to file " << logFile << std::endl;

                logger::Logger::logToFile(logFile);
            }
        });

        return parse2();
    }

    static void createCommandLineOptions(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode) {
        for (const CLI::Option* option : app->get_options()) {
            if (option->get_configurable()) {
                std::string value;

                if (option->reduced_results().size() > 0 && ((option->get_required() || mode == CLI::CallForCommandline::Mode::ALL) ||
                                                             mode == CLI::CallForCommandline::Mode::CONFIGURED)) {
                    value = option->reduced_results()[0];
                } else if (!option->get_default_str().empty()) {
                    value = (mode == CLI::CallForCommandline::Mode::ALL || option->get_required()) ? option->get_default_str() : "";
                } else if (option->get_required()) {
                    value = "<REQUIRED>";
                } else {
                    value = mode == CLI::CallForCommandline::Mode::ALL ? "\"\"" : "";
                }

                if (value.empty() && option->count() > 0 &&
                    (mode == CLI::CallForCommandline::Mode::ALL || mode == CLI::CallForCommandline::Mode::CONFIGURED)) {
                    value = "\"\"";
                }

                if (!value.empty()) {
                    out << "--" << option->get_single_name() << ((option->get_items_expected_max() == 0) ? "=" : " ") << value << " ";
                }
            }
        }
    }

    static std::string createCommandLineOptions(CLI::App* app, CLI::CallForCommandline::Mode mode) {
        std::stringstream out;

        createCommandLineOptions(out, app, mode);

        std::string optionString = out.str();
        if (optionString.back() == ' ') {
            optionString.pop_back();
        }

        return optionString;
    }

    static std::string createCommandLineSubcommands(CLI::App* app, CLI::CallForCommandline::Mode mode) {
        std::stringstream out;

        for (CLI::App* subcommand : app->get_subcommands({})) {
            if (!subcommand->get_name().empty()) {
                createCommandLineTemplate(out, subcommand, mode);
            } else {
                std::cout << subcommand->get_description() << std::endl;
            }
        }

        return out.str();
    }

    static void createCommandLineTemplate(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode) {
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

    static std::string createCommandLineTemplate(CLI::App* app, CLI::CallForCommandline::Mode mode) {
        std::stringstream out;

        createCommandLineTemplate(out, app, mode);

        std::string outString = out.str();
        while (app->get_parent() != nullptr) {
            app = app->get_parent();
            const std::string parentOptions = createCommandLineOptions(app, mode);
            outString = app->get_name() + " " + (!parentOptions.empty() ? parentOptions + " " : "") + outString;
        }

        if (outString.empty()) {
            outString = Config::getApplicationName();
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

    void Config::parse1() {
        app.allow_extras();

        try {
            app.parse(argc, argv);

            if (!quietOpt->as<bool>()) {
                logger::Logger::setLogLevel(logLevelOpt->as<int>());
                logger::Logger::setVerboseLevel(verboseLevelOpt->as<int>());
            } else {
                logger::Logger::quiet();
            }
        } catch (const CLI::ParseError&) {
            // Do not process ParseError here but on second parse pass
        }

        app.allow_extras(false);

        app.add_flag_callback( //
               "-h,--help",
               []() {
                   throw CLI::CallForHelp();
               },
               "Print this help message") //
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        app.add_flag_callback( //
               "-a,--help-all",
               []() {
                   throw CLI::CallForAllHelp();
               },
               "Print this help message, expand instances")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();
    }

    bool Config::parse2() {
        bool completed = false;

        try {
            try {
                app.parse(argc, argv);
                if (app["--show-config"]->count() > 0) {
                    throw CLI::CallForShowConfig();
                }
                if (app["--write-config"]->count() > 0) {
                    throw CLI::CallForWriteConfig(app["--write-config"]->as<std::string>());
                }
                completed = true;
            } catch (const CLI::CallForHelp&) {
                std::cout << app.help() << std::endl;
            } catch (const CLI::CallForAllHelp&) {
                std::cout << app.help("", CLI::AppFormatMode::All) << std::endl;
            } catch (const CLI::CallForVersion&) {
                std::cout << "SNode.C-Version: " << app.version() << std::endl << std::endl;
            } catch (const CLI::CallForCommandline& e) {
                std::cout << e.what();
                if (e.getMode() == CLI::CallForCommandline::Mode::REQUIRED) {
                    std::cout << "* Required options show either their configured value or <REQUIRED>" << std::endl;
                } else if (e.getMode() == CLI::CallForCommandline::Mode::CONFIGURED) {
                    std::cout << "* Required but not yet configured options show <REQUIRED> as value" << std::endl;
                    std::cout << "* Configured options show their configured value" << std::endl;
                } else if (e.getMode() == CLI::CallForCommandline::Mode::ALL) {
                    std::cout << "* Required but not yet configured options show <REQUIRED> as value " << std::endl;
                    std::cout << "* Remaining options show either their default or configured value" << std::endl;
                }
                std::cout << "* Options marked as <REQUIRED> need to be configured for a successful bootstrap" << std::endl;
                std::cout << std::endl
                          << Color::Code::FG_GREEN << "command@line" << Color::Code::FG_DEFAULT << ":" << Color::Code::FG_BLUE << "~/> "
                          << Color::Code::FG_DEFAULT << createCommandLineTemplate(e.getApp(), e.getMode()) << std::endl;
            } catch (const CLI::CallForShowConfig& e) {
                std::cout << e.what() << std::endl;
                try {
                    std::cout << app.config_to_str(true, true);
                } catch (const CLI::ParseError& e1) {
                    std::cout << "Error showing config file: " << e1.get_name() << " " << e1.what() << std::endl;
                    throw;
                }
            } catch (const CLI::CallForWriteConfig& e) {
                std::cout << e.what() << std::endl;
                std::ofstream confFile(e.getConfigFile());
                if (confFile.is_open()) {
                    try {
                        confFile << app.config_to_str(true, true);
                        confFile.close();
                    } catch (const CLI::ParseError& e1) {
                        std::cout << "Error writing config file: " << e1.get_name() << " " << e1.what() << std::endl;
                        throw;
                    }
                } else {
                    std::cout << "Error writing config file: " << std::strerror(errno) << std::endl;
                }
            } catch (const CLI::ConversionError& e) {
                std::cout << "Command line conversion error: " << e.what() << std::endl;
                throw;
            } catch (const CLI::ArgumentMismatch& e) {
                std::cout << "Command line error: Argument for " << e.what() << std::endl;
                throw;
            } catch (const CLI::ConfigError& e) {
                std::cout << "Config file (INI) parse error: " << e.get_name() << e.what() << std::endl;
                throw;
            } catch (const CLI::ParseError& e) {
                const std::string what = e.what();
                if (what.find("[Option Group: ]") != std::string::npos) { // If CLI11 throws that error it means for us there are
                                                                          // unconfigured anonymous instances
                    std::cout << "Bootstrap error: Anonymous instance(s) not configured in source code!" << std::endl;
                } else {
                    std::cout << "Command line error: " << what << std::endl;
                }
                throw;
            }
        } catch (const CLI::ParseError&) {
            std::cout << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
        } catch (const CLI::Error& e) {
            std::cout << "Error: " << e.get_name() << " " << e.what() << std::endl;
            std::cout << "Append -h, --help, or --help-all to your command line for more information." << std::endl;
        }

        return completed;
    }

    void Config::terminate() {
        if (app["--daemonize"]->as<bool>()) {
            std::ifstream pidFile(pidDirectory + "/" + applicationName + ".pid", std::ifstream::in);

            if (pidFile.good()) {
                pid_t pid = 0;
                pidFile >> pid;

                if (getpid() == pid) {
                    Daemon::erasePidFile(pidDirectory + "/" + applicationName + ".pid");
                }
            }
        } else if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) >= 0) {
            char buf[1024];
            while (read(STDIN_FILENO, buf, 1024) > 0)
                ;
        }
    }

    CLI::App* Config::add_instance(const std::string& name, const std::string& description) {
        CLI::App* instance = app.add_subcommand(name, description) //
                                 ->group("Instances")
                                 ->fallthrough()
                                 ->formatter(sectionFormatter)
                                 ->configurable(false)
                                 ->allow_extras(false)
                                 ->disabled(name.empty());
        instance //
            ->option_defaults()
            ->configurable(!name.empty());

        if (!aliases.empty()) {
            if (aliases.contains(name)) {
                instance //
                    ->alias(aliases[name]);
            } else {
                instance //
                    ->option_defaults()
                    ->take_first();
            }
        }

        instance //
            ->add_flag_callback(
                "--commandline",
                [instance]() {
                    throw CLI::CallForCommandline(instance, CLI::CallForCommandline::Mode::REQUIRED);
                },
                "Print a template command line showing required options only")
            ->configurable(false)
            ->disable_flag_override();

        instance //
            ->add_flag_callback(
                "--commandline-full",
                [instance]() {
                    throw CLI::CallForCommandline(instance, CLI::CallForCommandline::Mode::ALL);
                },
                "Print a template command line showing all possible options")
            ->configurable(false)
            ->disable_flag_override();

        instance //
            ->add_flag_callback(
                "--commandline-configured",
                [instance]() {
                    throw CLI::CallForCommandline(instance, CLI::CallForCommandline::Mode::CONFIGURED);
                },
                "Print a template command line showing all required and configured options") //
            ->configurable(false)
            ->disable_flag_override();

        instance->set_help_flag();

        instance //
            ->add_flag_callback(
                "-h,--help",
                []() {
                    throw CLI::CallForHelp();
                },
                "Print this help message")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        instance //
            ->add_flag_callback(
                "-a,--help-all",
                []() {
                    throw CLI::CallForAllHelp();
                },
                "Print this help message, expand sections")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        return instance;
    }

    void Config::required(CLI::App* instance, bool req) {
        if (req) {
            app.needs(instance);
        } else {
            app.remove_needs(instance);
        }

        instance->required(req);
    }

    bool Config::remove_instance(CLI::App* instance) {
        Config::required(instance, false);

        return app.remove_subcommand(instance);
    }

    void Config::add_string_option(const std::string& name, const std::string& description, const std::string& typeName) {
        applicationOptions[name] = app //
                                       .add_option_function<std::string>(name, utils::ResetToDefault(applicationOptions[name]), description)
                                       ->take_last()
                                       ->type_name(typeName)
                                       ->configurable()
                                       ->required()
                                       ->group("Application Options");

        app.needs(applicationOptions[name]);
    }

    void Config::add_string_option(const std::string& name,
                                   const std::string& description,
                                   const std::string& typeName,
                                   const std::string& defaultValue) {
        add_string_option(name, description, typeName);

        applicationOptions[name] //
            ->required(false)
            ->default_val(defaultValue);

        app.remove_needs(applicationOptions[name]);
    }

    void Config::add_string_option(
        const std::string& name, const std::string& description, const std::string& typeName, const char* defaultValue, bool configurable) {
        add_string_option(name, description, typeName, std::string(defaultValue), configurable);
    }

    void Config::add_string_option(const std::string& name,
                                   const std::string& description,
                                   const std::string& typeName,
                                   const char* defaultValue) {
        add_string_option(name, description, typeName, std::string(defaultValue));
    }

    void
    Config::add_string_option(const std::string& name, const std::string& description, const std::string& typeName, bool configurable) {
        add_string_option(name, description, typeName);
        applicationOptions[name] //
            ->configurable(configurable);
    }

    void Config::add_string_option(const std::string& name,
                                   const std::string& description,
                                   const std::string& typeName,
                                   const std::string& defaultValue,
                                   bool configurable) {
        add_string_option(name, description, typeName, defaultValue);
        applicationOptions[name] //
            ->configurable(configurable);
    }

    std::string Config::get_string_option_value(const std::string& name) {
        if (app.get_option(name) == nullptr) {
            throw CLI::OptionNotFound(name);
        }

        return app[name]->as<std::string>();
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
