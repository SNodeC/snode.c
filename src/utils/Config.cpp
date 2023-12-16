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

#include "log/Logger.h"
#include "utils/Exceptions.h"

#include <cerrno>
#include <cstdint>
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

namespace utils {

    static std::shared_ptr<CLI::App> makeApp() { // NO_LINT
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();

        app->configurable(false);
        app->allow_extras();
        app->allow_config_extras();

        const std::shared_ptr<CLI::HelpFormatter> helpFormatter = std::make_shared<CLI::HelpFormatter>();

        helpFormatter->label("SUBCOMMAND", "INSTANCE");
        helpFormatter->label("SUBCOMMANDS", "INSTANCES");
        helpFormatter->label("PERSISTENT", "");
        helpFormatter->label("Persistent Options", "Options (persistent)");
        helpFormatter->label("Nonpersistent Options", "Options (nonpersistent)");
        helpFormatter->label("Usage", "\nUsage");
        helpFormatter->label("bool:{true,false}", "{true,false}");
        helpFormatter->label(":{standard,required,full,default}", "{standard,required,full,default}");
        helpFormatter->label(":{standard,expanded}", "{standard,expanded}");
        helpFormatter->column_width(7);

        app->formatter(helpFormatter);

        app->config_formatter(std::make_shared<CLI::ConfigFormatter>());

        app->option_defaults()->take_last();
        app->option_defaults()->group(app->get_formatter()->get_label("Nonpersistent Options"));

        logger::Logger::init();

        return app;
    }

    std::shared_ptr<CLI::App> Config::app = makeApp();

    bool Config::init(int argc, char* argv[]) {
        bool proceed = true;

        Config::argc = argc;
        Config::argv = argv;

        applicationName = std::filesystem::path(argv[0]).filename();

        uid_t euid = 0;
        struct passwd* pw = nullptr;
        struct group* gr = nullptr;

        if ((pw = getpwuid(getuid())) == nullptr) {
            proceed = false;
        } else if ((gr = getgrgid(pw->pw_gid)) == nullptr) {
            proceed = false;
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
                proceed = false;
            }
        }

        if (proceed && !std::filesystem::exists(configDirectory)) {
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
                        proceed = false;
                    }
                }
            } else {
                std::cout << "Error: Can not create directory '" << configDirectory << "'" << std::endl;
                proceed = false;
            }
        }

        if (proceed && !std::filesystem::exists(logDirectory)) {
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
                        proceed = false;
                    }
                }
            } else {
                std::cout << "Error: Can not create directory '" << logDirectory << "'" << std::endl;
                proceed = false;
            }
        }

        if (proceed && !std::filesystem::exists(pidDirectory)) {
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
                        proceed = false;
                    }
                }
            } else {
                std::cout << "Error: Can not create directory '" << pidDirectory << "'" << std::endl;
                proceed = false;
            }
        }

        if (proceed) {
            app->description("Configuration for Application '" + applicationName + "'");

            app->footer("Application '" + applicationName +
                        "' powered by SNode.C\n"
                        "(C) 2019-2023 Volker Christian\n"
                        "https://github.com/VolkerChristian/snode.c - me@vchrist.at");

            app->set_config( //
                   "-c,--config-file",
                   configDirectory + "/" + applicationName + ".conf",
                   "Read a config file",
                   false) //
                ->take_all()
                ->type_name("configfile")
                ->check(!CLI::ExistingDirectory);

            app->add_option("-w,--write-config",
                            "Write config file and exit") //
                ->configurable(false)
                ->default_val(configDirectory + "/" + applicationName + ".conf")
                ->type_name("[configfile]")
                ->check(!CLI::ExistingDirectory)
                ->expected(0, 1);

            app->add_flag( //
                   "-k,--kill",
                   "Kill running daemon") //
                ->configurable(false)
                ->disable_flag_override();

            app->add_option( //
                   "-i,--instance-alias",
                   "Make an instance also known as an alias in configuration files")
                ->configurable(false)
                ->type_name("instance=instance_alias [instance=instance_alias [...]]")
                ->each([](const std::string& item) -> void {
                    const auto it = item.find('=');
                    if (it != std::string::npos) {
                        aliases[item.substr(0, it)] = item.substr(it + 1);
                    } else {
                        throw CLI::ConversionError("Can not convert '" + item + "' to a 'instance=instance_alias' pair");
                    }
                });

            add_standard_flags(app.get());

            logLevelOpt = app->add_option_function<std::string>( //
                                 "-l,--log-level",
                                 utils::ResetToDefault(logLevelOpt),
                                 "Log level") //
                              ->default_val(4)
                              ->type_name("level")
                              ->check(CLI::Range(0, 6))
                              ->group(app->get_formatter()->get_label("Persistent Options"));

            verboseLevelOpt = app->add_option_function<std::string>( //
                                     "-v,--verbose-level",
                                     utils::ResetToDefault(verboseLevelOpt),
                                     "Verbose level") //
                                  ->default_val(1)
                                  ->type_name("level")
                                  ->check(CLI::Range(0, 10))
                                  ->group(app->get_formatter()->get_label("Persistent Options"));

            quietOpt = app->add_flag_function( //
                              "-q{true},!-u,--quiet{true}",
                              utils::ResetToDefault(quietOpt),
                              "Quiet mode") //
                           ->take_last()
                           ->default_val("false")
                           ->type_name("bool")
                           ->check(CLI::IsMember({"true", "false"}))
                           ->group(app->get_formatter()->get_label("Persistent Options"));

            logFileOpt = app->add_option_function<std::string>( //
                                "--log-file",
                                utils::ResetToDefault(logFileOpt),
                                "Log file path") //
                             ->default_val(logDirectory + "/" + applicationName + ".log")
                             ->type_name("logfile")
                             ->check(!CLI::ExistingDirectory)
                             ->group(app->get_formatter()->get_label("Persistent Options"));

            enforceLogFileOpt = app->add_flag_function( //
                                       "-e{true},!-n,--enforce-log-file{true}",
                                       utils::ResetToDefault(enforceLogFileOpt),
                                       "Enforce writing of logs to file for foreground applications") //
                                    ->take_last()
                                    ->default_val("false")
                                    ->type_name("bool")
                                    ->check(CLI::IsMember({"true", "false"}))
                                    ->group(app->get_formatter()->get_label("Persistent Options"));

            daemonizeOpt = app->add_flag_function( //
                                  "-d{true},!-f,--daemonize{true}",
                                  utils::ResetToDefault(daemonizeOpt),
                                  "Start application as daemon") //
                               ->take_last()
                               ->default_val("false")
                               ->type_name("bool")
                               ->check(CLI::IsMember({"true", "false"}))
                               ->group(app->get_formatter()->get_label("Persistent Options"));

            userNameOpt = app->add_option_function<std::string>( //
                                 "--user-name",
                                 utils::ResetToDefault(userNameOpt),
                                 "Run daemon under specific user permissions") //
                              ->default_val(pw->pw_name)
                              ->type_name("username")
                              ->needs(daemonizeOpt)
                              ->group(app->get_formatter()->get_label("Persistent Options"));

            groupNameOpt = app->add_option_function<std::string>( //
                                  "--group-name",
                                  utils::ResetToDefault(groupNameOpt),
                                  "Run daemon under specific group permissions")
                               ->default_val(gr->gr_name)
                               ->type_name("groupname")
                               ->needs(daemonizeOpt)
                               ->group(app->get_formatter()->get_label("Persistent Options"));

            proceed = parse1(); // for stopDaemon and pre init application options

            add_help_flags(app.get());
        }

        return proceed;
    }

    bool Config::bootstrap() {
        aliases.clear();

        app->final_callback([]() -> void {
            if (daemonizeOpt->as<bool>() && (*app)["--show-config"]->count() == 0 && (*app)["--write-config"]->count() == 0 &&
                (*app)["--command-line"]->count() == 0) {
                std::cout << "Running as daemon (double fork)" << std::endl;

                utils::Daemon::startDaemon(
                    pidDirectory + "/" + applicationName + ".pid", userNameOpt->as<std::string>(), groupNameOpt->as<std::string>());

                logger::Logger::setQuiet();

                const std::string logFile = logFileOpt->as<std::string>();
                if (!logFile.empty()) {
                    logger::Logger::logToFile(logFile);
                }
            } else if ((*app)["--enforce-log-file"]->as<bool>()) {
                const std::string logFile = logFileOpt->as<std::string>();
                if (!logFile.empty()) {
                    std::cout << "Writing logs to file " << logFile << std::endl;

                    logger::Logger::logToFile(logFile);
                }
            }
        });

        return parse2();
    }

    bool Config::parse1() {
        bool proceed = true;

        try {
            app->parse(argc, argv);
        } catch (const CLI::ParseError&) {
            // Do not process ParseError here but on second parse pass
        }

        if ((*app)["--kill"]->count() > 0) {
            try {
                utils::Daemon::stopDaemon(pidDirectory + "/" + applicationName + ".pid");
            } catch (const DaemonError& e) {
                std::cout << "DaemonError: " << e.what() << std::endl;
            } catch (const DaemonFailure& e) {
                std::cout << "DaemonFailure: " << e.what() << std::endl;
            } catch (const DaemonExited& e) {
                std::cout << "Daemon terminated: " << e.what() << ": " << e.getPid() << std::endl;
            }

            proceed = false;
        } else {
            if ((*app)["--show-config"]->count() == 0 && (*app)["--write-config"]->count() == 0 && (*app)["--command-line"]->count() == 0) {
                app->allow_extras(false);
            }

            if (!quietOpt->as<bool>()) {
                logger::Logger::setLogLevel(logLevelOpt->as<int>());
                logger::Logger::setVerboseLevel(verboseLevelOpt->as<int>());
            }

            logger::Logger::setQuiet(quietOpt->as<bool>());
        }

        return proceed;
    }

    static void createCommandLineOptions(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode) {
        CLI::Option* disabledOpt = app->get_option_no_throw("--disabled");
        const bool disabled = disabledOpt != nullptr ? disabledOpt->as<bool>() : false;
        if (!disabled || mode == CLI::CallForCommandline::Mode::DEFAULT || mode == CLI::CallForCommandline::Mode::FULL) {
            for (const CLI::Option* option : app->get_options()) {
                if (option->get_configurable()) {
                    std::string value;

                    switch (mode) {
                        case CLI::CallForCommandline::Mode::NONEDEFAULT:
                            if (option->count() > 0) {
                                value = option->as<std::string>();
                            } else if (option->get_required()) {
                                value = "<REQUIRED>";
                            }
                            break;
                        case CLI::CallForCommandline::Mode::REQUIRED:
                            if (option->get_required()) {
                                if (option->count() > 0) {
                                    value = option->as<std::string>();
                                } else {
                                    value = "<REQUIRED>";
                                }
                            }
                            break;
                        case CLI::CallForCommandline::Mode::FULL:
                            if (option->count() > 0) {
                                value = option->as<std::string>();
                            } else if (!option->get_default_str().empty()) {
                                value = option->get_default_str();
                            } else if (!option->get_required()) {
                                value = "\"\"";
                            } else {
                                value = "<REQUIRED>";
                            }
                            break;
                        case CLI::CallForCommandline::Mode::DEFAULT: {
                            if (!option->get_default_str().empty()) {
                                value = option->get_default_str();
                            } else if (!option->get_required()) {
                                value = "\"\"";
                            } else {
                                value = "<REQUIRED>";
                            }
                            break;
                        }
                    }

                    if (!value.empty()) {
                        out << "--" << option->get_single_name() << ((option->get_items_expected_max() == 0) ? "=" : " ") << value << " ";
                    }
                }
            }
        } else if (disabledOpt->get_default_str() == "false") {
            out << "--disabled=true ";
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

    static void createCommandLineTemplate(std::stringstream& out, CLI::App* app, CLI::CallForCommandline::Mode mode);

    static std::string createCommandLineSubcommands(CLI::App* app, CLI::CallForCommandline::Mode mode) {
        std::stringstream out;

        CLI::Option* disabledOpt = app->get_option_no_throw("--disabled");
        if (disabledOpt == nullptr || !disabledOpt->as<bool>() || mode == CLI::CallForCommandline::Mode::DEFAULT ||
            mode == CLI::CallForCommandline::Mode::FULL) {
            for (CLI::App* subcommand : app->get_subcommands({})) {
                if (!subcommand->get_name().empty()) {
                    createCommandLineTemplate(out, subcommand, mode);
                }
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
            outString = std::string(app->get_name())
                            .append(" ")
                            .append(!parentOptions.empty() ? std::string(parentOptions).append(" ") : "")
                            .append(outString);
        }

        if (outString.empty()) {
            outString = Config::getApplicationName();
        }

        return outString;
    }

    bool Config::parse2() {
        bool success = false;

        try {
            try {
                app->parse(argc, argv);

                if ((*app)["--write-config"]->count() > 0) {
                    throw CLI::CallForWriteConfig((*app)["--write-config"]->as<std::string>());
                }

                success = true;
            } catch (const DaemonError& e) {
                std::cout << "Daemon error: " << e.what() << " ... exiting" << std::endl;
            } catch (const DaemonFailure& e) {
                std::cout << "Daemon failure: " << e.what() << " ... exiting" << std::endl;
            } catch (const DaemonExited& e) {
                std::cout << "Parent finished: " << e.what() << ": " << e.getPid() << std::endl;
            } catch (const CLI::CallForHelp&) {
                std::cout << app->help() << std::endl;
            } catch (const CLI::CallForAllHelp&) {
                std::cout << app->help("", CLI::AppFormatMode::All) << std::endl;
            } catch (const CLI::CallForVersion&) {
                std::cout << "SNode.C-Version: " << app->version() << std::endl << std::endl;
            } catch (const CLI::CallForCommandline& e) {
                std::cout << e.what() << ":" << std::endl;
                std::cout << std::endl
                          << Color::Code::FG_GREEN << "command@line" << Color::Code::FG_DEFAULT << ":" << Color::Code::FG_BLUE << "~/> "
                          << Color::Code::FG_DEFAULT << createCommandLineTemplate(e.getApp(), e.getMode()) << std::endl
                          << std::endl;
                std::cout << "* Options show either their configured or default value" << std::endl;
                std::cout << "* Required but not yet configured options show <REQUIRED> as value" << std::endl;
                std::cout << "* Options marked as <REQUIRED> need to be configured for a successful bootstrap" << std::endl;
            } catch (const CLI::CallForShowConfig& e) {
                try {
                    std::cout << e.getApp()->config_to_str(true, true);
                } catch (const CLI::ParseError& e1) {
                    std::cout << "Error showing config file: " << e.getApp() << " " << e1.get_name() << " " << e1.what() << std::endl;
                    throw;
                }
            } catch (const CLI::CallForWriteConfig& e) {
                std::cout << e.what() << std::endl;
                std::ofstream confFile(e.getConfigFile());
                if (confFile.is_open()) {
                    try {
                        confFile << app->config_to_str(true, true);
                        confFile.close();
                    } catch (const CLI::ParseError& e1) {
                        confFile.close();
                        std::cout << "Error writing config file: " << e1.get_name() << " " << e1.what() << std::endl;
                        throw;
                    }
                } else {
                    std::cout << "Error writing config file: " << std::strerror(errno) << std::endl;
                }
            } catch (const CLI::ConversionError& e) {
                std::cout << Color::Code::FG_RED << "[" << e.get_name() << "] " << Color::Code::FG_DEFAULT << e.what() << std::endl;
                throw;
            } catch (const CLI::ArgumentMismatch& e) {
                std::cout << Color::Code::FG_RED << "[" << e.get_name() << "] " << Color::Code::FG_DEFAULT << e.what() << std::endl;
                throw;
            } catch (const CLI::ConfigError& e) {
                std::cout << Color::Code::FG_RED << "[" << e.get_name() << "] " << Color::Code::FG_DEFAULT << e.what() << std::endl;
                std::cout << "              Adding '-w' on the command line may solve this problem" << std::endl;
                throw;
            } catch (const CLI::ParseError& e) {
                const std::string what = e.what();
                if (what.find("[Option Group: ") != std::string::npos) { // If CLI11 throws that error it means for us there are
                                                                         // unconfigured anonymous instances
                    std::cout << Color::Code::FG_RED << "[BootstrapError]" << Color::Code::FG_DEFAULT
                              << " Anonymous instance(s) not configured in source code " << std::endl;
                } else {
                    std::cout << Color::Code::FG_RED << "[" << e.get_name() << "] " << Color::Code::FG_DEFAULT << what << std::endl;
                }
                throw;
            }
        } catch (const CLI::ParseError&) {
            std::cout << std::endl << "Append -h or --help  to your command line for more information." << std::endl;
        } catch (const CLI::Error& e) {
            std::cout << "Error: " << e.get_name() << " " << e.what() << std::endl;
            std::cout << "Append -h or --help to your command line for more information." << std::endl;
        }

        if (!success) { // cppcheck-suppress knownConditionTrueFalse
            logger::Logger::setQuiet();
        }

        return success;
    }

    void Config::terminate() {
        if ((*app)["--daemonize"]->as<bool>()) {
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
            while (read(STDIN_FILENO, buf, 1024) > 0) {
            }
        }
    }

    static std::shared_ptr<CLI::HelpFormatter> makeSectionFormatter() {
        std::shared_ptr<CLI::HelpFormatter> sectionFormatter = std::make_shared<CLI::HelpFormatter>();

        sectionFormatter->label("SUBCOMMAND", "SECTION");
        sectionFormatter->label("SUBCOMMANDS", "SECTIONS");
        sectionFormatter->label("PERSISTENT", "");
        sectionFormatter->label("Persistent Options", "Options (persistent)");
        sectionFormatter->label("Nonpersistent Options", "Options (nonpersistent)");
        sectionFormatter->label("Usage", "\nUsage");
        sectionFormatter->label("bool:{true,false}", "{true,false}");
        sectionFormatter->label(":{standard,required,full,default}", "{standard,required,full,default}");
        sectionFormatter->label(":{standard,expanded}", "{standard,expanded}");
        sectionFormatter->column_width(7);

        return sectionFormatter;
    }

    std::shared_ptr<CLI::Formatter> Config::sectionFormatter = makeSectionFormatter();

    CLI::App* Config::add_instance(const std::string& name, const std::string& description, const std::string& group) {
        CLI::App* instance = app->add_subcommand(name, description) //
                                 ->group(group)
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

        return instance;
    }

    CLI::App* Config::add_standard_flags(CLI::App* app) {
        app->add_flag_callback( //
               "-s,--show-config",
               [app]() {
                   throw CLI::CallForShowConfig(app);
               },
               "Show current configuration and exit") //
            ->configurable(false)
            ->disable_flag_override();

        app //
            ->add_flag_function(
                "--command-line{standard}",
                [app]([[maybe_unused]] std::int64_t count) {
                    const std::string& result = app->get_option("--command-line")->as<std::string>();
                    if (result == "standard") {
                        throw CLI::CallForCommandline(app,
                                                      "Below is a command line viewing all non-default and required options",
                                                      CLI::CallForCommandline::Mode::NONEDEFAULT);
                    }
                    if (result == "required") {
                        throw CLI::CallForCommandline(
                            app, "Below is a command line viewing required options only", CLI::CallForCommandline::Mode::REQUIRED);
                    }
                    if (result == "full") {
                        throw CLI::CallForCommandline(
                            app,
                            "Below is a command line viewing the full set of options with their default or configured values",
                            CLI::CallForCommandline::Mode::FULL);
                    }
                    if (result == "default") {
                        throw CLI::CallForCommandline(app,
                                                      "Below is a command line viewing the full set of options with their default values",
                                                      CLI::CallForCommandline::Mode::DEFAULT);
                    }
                },
                "Print a command line\n"
                "  standard (default): View all non-default and required options\n"
                "  required: View required options only\n"
                "  full: View the full set of options with their default or configured values\n"
                "  default: View the full set of options with their default values")
            ->take_last()
            ->configurable(false)
            ->check(CLI::IsMember({"standard", "required", "full", "default"}));

        return app;
    }

    CLI::App* Config::add_help_flags(CLI::App* app) {
        app //
            ->set_help_flag();

        app //
            ->add_flag_function(
                "-h{standard},--help{standard}",
                [app]([[maybe_unused]] std::int64_t count) {
                    const std::string& result = app->get_option("--help")->as<std::string>();

                    if (result == "standard") {
                        throw CLI::CallForHelp();
                    }
                    if (result == "expanded") {
                        throw CLI::CallForAllHelp();
                    }
                },
                "Print help message")
            ->take_last()
            ->configurable(false)
            ->check(CLI::IsMember({"standard", "expanded"}))
            ->trigger_on_parse();

        return app;
    }

    CLI::App* Config::add_help_flag(CLI::App* app) {
        app //
            ->set_help_flag();

        app //
            ->add_flag_function(
                "-h,--help",
                []([[maybe_unused]] std::int64_t count) {
                    throw CLI::CallForHelp();
                },
                "Print help message")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        return app;
    }

    void Config::required(CLI::App* instance, bool req) {
        if (req) {
            app->needs(instance);
        } else {
            app->remove_needs(instance);
        }

        instance->required(req);
    }

    bool Config::remove_instance(CLI::App* instance) {
        Config::required(instance, false);

        return app->remove_subcommand(instance);
    }

    CLI::Option* Config::add_string_option(const std::string& name, const std::string& description, const std::string& typeName) {
        applicationOptions[name] =
            app //
                ->add_option_function<std::string>(name, utils::ResetToDefault(applicationOptions[name]), description)
                ->take_last()
                ->type_name(typeName)
                ->configurable()
                ->required()
                ->group("Application Options");

        app->needs(applicationOptions[name]);

        return applicationOptions[name];
    }

    CLI::Option*
    Config::add_string_option(const std::string& name, const std::string& description, const std::string& typeName, bool configurable) {
        add_string_option(name, description, typeName);
        return applicationOptions[name] //
            ->configurable(configurable);
    }

    CLI::Option* Config::add_string_option(const std::string& name,
                                           const std::string& description,
                                           const std::string& typeName,
                                           const std::string& defaultValue) {
        add_string_option(name, description, typeName);

        applicationOptions[name] //
            ->required(false)
            ->default_val(defaultValue);

        app->remove_needs(applicationOptions[name]);

        return applicationOptions[name];
    }

    CLI::Option* Config::add_string_option(const std::string& name,
                                           const std::string& description,
                                           const std::string& typeName,
                                           const std::string& defaultValue,
                                           bool configurable) {
        add_string_option(name, description, typeName, defaultValue);
        return applicationOptions[name] //
            ->configurable(configurable);
    }

    CLI::Option* Config::add_string_option(const std::string& name,
                                           const std::string& description,
                                           const std::string& typeName,
                                           const char* defaultValue) {
        return add_string_option(name, description, typeName, std::string(defaultValue));
    }

    CLI::Option* Config::add_string_option(
        const std::string& name, const std::string& description, const std::string& typeName, const char* defaultValue, bool configurable) {
        return add_string_option(name, description, typeName, std::string(defaultValue), configurable);
    }

    std::string Config::get_string_option_value(const std::string& name) {
        if (app->get_option(name) == nullptr) {
            throw CLI::OptionNotFound(name);
        }

        return (*app)[name]->as<std::string>();
    }

    void Config::add_flag(const std::string& name,
                          bool& variable,
                          const std::string& description,
                          bool required,
                          bool configurable,
                          const std::string& groupName) {
        app->add_flag(name, variable, description) //
            ->required(required)                   //
            ->configurable(configurable)           //
            ->group(groupName);
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

    int Config::argc = 0;
    char** Config::argv = nullptr;

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

    std::map<std::string, std::string> Config::aliases;
    std::map<std::string, CLI::Option*> Config::applicationOptions;

} // namespace utils
