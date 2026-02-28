/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "utils/Config.h"

#include "utils/Daemon.h"
#include "utils/Formatter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <grp.h>
#include <iostream>
#include <memory>
#include <pwd.h>
#include <sstream>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#if __has_warning("-Wmissing-noreturn")
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
#if __has_warning("-Wnrvo")
#pragma GCC diagnostic ignored "-Wnrvo"
#endif
#endif
#endif
#include "utils/CLI11.hpp"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define XSTR(s) STR(s)
#define STR(s) #s

namespace utils {

    std::vector<std::shared_ptr<void>> Config::configInstances;

    static std::shared_ptr<CLI::App> makeApp() { // NO_LINT
        const std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();

        app->configurable(false);
        app->allow_extras();

        const std::shared_ptr<CLI::HelpFormatter> helpFormatter = std::make_shared<CLI::HelpFormatter>();

        helpFormatter->label("SUBCOMMAND", "INSTANCE");
        helpFormatter->label("SUBCOMMANDS", "INSTANCES");
        helpFormatter->label("PERSISTENT", "");
        helpFormatter->label("Persistent Options", "Options (persistent)");
        helpFormatter->label("Nonpersistent Options", "Options (nonpersistent)");
        helpFormatter->label("Usage", "\nUsage");
        helpFormatter->label("bool:{true,false}", "{true,false}");
        helpFormatter->label(":{standard,active,complete,required}", "{standard,active,complete,required}");
        helpFormatter->label(":{standard,exact,expanded}", "{standard,exact,expanded}");
        helpFormatter->column_width(7);

        app->formatter(helpFormatter);

        app->config_formatter(std::make_shared<CLI::ConfigFormatter>());
        app->get_config_formatter_base()->arrayDelimiter(' ');

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
        const struct passwd* pw = nullptr;
        const struct group* gr = nullptr;

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
                    const struct group* gr = nullptr;
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
                    const struct group* gr = nullptr;
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
                    const struct group* gr = nullptr;
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
                        "(C) 2020-2025 Volker Christian <me@vchrist.at>\n"
                        "https://github.com/SNodeC/snode.c");

            app->set_config( //
                   "-c,--config-file",
                   configDirectory + "/" + applicationName + ".conf",
                   "Read a config file",
                   false) //
                ->take_all()
                ->type_name("configfile")
                ->check(!CLI::ExistingDirectory);

            app->add_option("-w,--write-config", "Write config file and exit")
                ->configurable(false)
                ->default_val(configDirectory + "/" + applicationName + ".conf")
                ->type_name("configfile")
                ->check(!CLI::ExistingDirectory)
                ->expected(0, 1);

            addStandardFlags(app.get());

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
                ->each([](const std::string& item) {
                    const auto it = item.find('=');
                    if (it != std::string::npos) {
                        aliases[item.substr(0, it)] = item.substr(it + 1);
                    } else {
                        throw CLI::ConversionError("Can not convert '" + item + "' to a 'instance=instance_alias' pair");
                    }
                });

            logLevelOpt = app->add_option( //
                                 "--log-level",
                                 "Log level") //
                              ->default_val(4)
                              ->type_name("level")
                              ->check(CLI::Range(0, 6))
                              ->group(app->get_formatter()->get_label("Persistent Options"));

            verboseLevelOpt = app->add_option( //
                                     "--verbose-level",
                                     "Verbose level") //
                                  ->default_val(2)
                                  ->type_name("level")
                                  ->check(CLI::Range(0, 10))
                                  ->group(app->get_formatter()->get_label("Persistent Options"));

            logFileOpt = app->add_option( //
                                "-l,--log-file",
                                "Log file path") //
                             ->default_val(logDirectory + "/" + applicationName + ".log")
                             ->type_name("logfile")
                             ->check(!CLI::ExistingDirectory)
                             ->group(app->get_formatter()->get_label("Persistent Options"));

            monochromLogOpt = app->add_flag(
                                     "-m{true},--monochrom-log{true}",
                                     []([[maybe_unused]] std::size_t count) {
                                         if (monochromLogOpt->as<bool>()) {
                                             logger::Logger::setDisableColor(true);
                                         } else {
                                             logger::Logger::setDisableColor(false);
                                         }
                                     },
                                     "Monochrom log output")
                                  ->default_val(logger::Logger::getDisableColor() ? "true" : "false")
                                  ->type_name("bool")
                                  ->check(CLI::IsMember({"true", "false"}))
                                  ->group(app->get_formatter()->get_label("Persistent Options"))
                                  ->trigger_on_parse();

            quietOpt = app->add_flag( //
                              "-q{true},--quiet{true}",
                              "Quiet mode") //
                           ->default_val("false")
                           ->type_name("bool")
                           ->check(CLI::IsMember({"true", "false"}))
                           ->group(app->get_formatter()->get_label("Persistent Options"));

            enforceLogFileOpt = app->add_flag( //
                                       "-e{true},--enforce-log-file{true}",
                                       "Enforce writing of logs to file for foreground applications") //
                                    ->default_val("false")
                                    ->type_name("bool")
                                    ->check(CLI::IsMember({"true", "false"}))
                                    ->group(app->get_formatter()->get_label("Persistent Options"));

            daemonizeOpt = app->add_flag( //
                                  "-d{true},--daemonize{true}",
                                  "Start application as daemon") //
                               ->default_val("false")
                               ->type_name("bool")
                               ->check(CLI::IsMember({"true", "false"}))
                               ->group(app->get_formatter()->get_label("Persistent Options"));

            userNameOpt = app->add_option( //
                                 "-u,--user-name",
                                 "Run daemon under specific user permissions") //
                              ->default_val(pw->pw_name)
                              ->type_name("username")
                              ->needs(daemonizeOpt)
                              ->group(app->get_formatter()->get_label("Persistent Options"));

            groupNameOpt = app->add_option( //
                                  "-g,--group-name",
                                  "Run daemon under specific group permissions")
                               ->default_val(gr->gr_name)
                               ->type_name("groupname")
                               ->needs(daemonizeOpt)
                               ->group(app->get_formatter()->get_label("Persistent Options"));

            versionOpt = app->set_version_flag("-v,--version", "1.0-rc1", "Framework version");

            addHelp(app.get());

            proceed = parse1(); // for stopDaemon and pre init application options
        }

        return proceed;
    }

    bool Config::bootstrap() {
        aliases.clear();

        app->final_callback([]() {
            if (daemonizeOpt->as<bool>() && helpTriggerApp == nullptr && showConfigTriggerApp == nullptr &&
                (*app)["--write-config"]->count() == 0 && (*app)["--command-line"]->count() == 0) {
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

    std::string Config::getApplicationName() {
        return applicationName;
    }

    int Config::getLogLevel() {
        return logLevelOpt->as<int>();
    }

    int Config::getVerboseLevel() {
        return verboseLevelOpt->as<int>();
    }

    namespace CallForCommandline {
        enum class Mode { REQUIRED, STANDARD, ACTIVE, COMPLETE };
    }

    static std::string createCommandLineTemplate(CLI::App* app, utils::CallForCommandline::Mode mode);

    // Escape characters with special meaning in Bash (except whitespace).
    static std::string bash_backslash_escape_no_whitespace(std::string_view s) {
        static const std::unordered_set<char> special{
            '\\', '\'',                                    // quoting/escape
            '`',  '$',                                     // substitution
            '|',  '&',  ';', '<', '>', '(', ')', '{', '}', // operators
            '*',  '?',  '[', ']', '~', '!', '#', '='       // globbing/history/others
        };

        std::string out;
        out.reserve(s.size() * 3);

        for (const char c : s) {
            if (special.contains(c)) {
                out.push_back('\\');
            }
            out.push_back(c);
        }
        return out;
    }

    static void createCommandLineOptions(std::stringstream& out, CLI::App* app, utils::CallForCommandline::Mode mode) {
        const CLI::Option* disabledOpt = app->get_option_no_throw("--disabled");
        const bool disabled = disabledOpt != nullptr ? disabledOpt->as<bool>() : false;
        if (!disabled || mode == utils::CallForCommandline::Mode::COMPLETE) {
            for (const CLI::Option* option : app->get_options()) {
                if (option->get_configurable()) {
                    std::string value;

                    switch (mode) {
                        case utils::CallForCommandline::Mode::STANDARD:
                            if (option->count() > 0) {
                                try {
                                    for (const auto& result : option->reduced_results()) {
                                        value += (!result.empty() ? result : "\"\"") + " ";
                                    }
                                    value.pop_back();
                                } catch (CLI::ParseError& e) {
                                    value = std::string{"<["} + Color::Code::FG_RED + e.get_name() + Color::Code::FG_DEFAULT + "] " +
                                            e.what() + ">";
                                }
                            } else if (option->get_required()) {
                                value = "<REQUIRED>";
                            }
                            break;
                        case utils::CallForCommandline::Mode::REQUIRED:
                            if (option->get_required()) {
                                if (option->count() > 0) {
                                    try {
                                        for (const auto& result : option->reduced_results()) {
                                            value += (!result.empty() ? result : "\"\"") + " ";
                                        }
                                        value.pop_back();
                                    } catch (CLI::ParseError& e) {
                                        value = std::string{"<["} + Color::Code::FG_RED + e.get_name() + Color::Code::FG_DEFAULT + "] " +
                                                e.what() + ">";
                                    }
                                } else {
                                    value = "<REQUIRED>";
                                }
                            }
                            break;
                        case utils::CallForCommandline::Mode::ACTIVE:
                        case utils::CallForCommandline::Mode::COMPLETE:
                            if (option->count() > 0) {
                                try {
                                    for (const auto& result : option->reduced_results()) {
                                        value += (!result.empty() ? result : "\"\"") + " ";
                                    }
                                    value.pop_back();
                                } catch (CLI::ParseError& e) {
                                    value = std::string{"<["} + Color::Code::FG_RED + e.get_name() + Color::Code::FG_DEFAULT + "] " +
                                            e.what() + ">";
                                }
                            } else if (!option->get_default_str().empty()) {
                                value = option->get_default_str();
                            } else if (!option->get_required()) {
                                value = "\"\"";
                            } else {
                                value = "<REQUIRED>";
                            }
                            break;
                    }

                    if (!value.empty()) {
                        if (value.starts_with(std::string{"["}) && value.ends_with("]")) {
                            value = value.substr(1, value.size() - 2);
                        }

                        if (value != "<REQUIRED>" && value != "\"\"" && !value.starts_with("<[")) {
                            value = bash_backslash_escape_no_whitespace(value);
                        }
                        out << "--" << option->get_single_name() << ((option->get_items_expected_max() == 0) ? "=" : " ") << value << " ";
                    }
                }
            }
        } else if (disabledOpt->get_default_str() == "false") {
            out << "--disabled=true ";
        }
    }

    static std::string createCommandLineOptions(CLI::App* app, utils::CallForCommandline::Mode mode) {
        std::stringstream out;

        createCommandLineOptions(out, app, mode);

        std::string optionString = out.str();
        if (!optionString.empty() && optionString.back() == ' ') {
            optionString.pop_back();
        }

        return optionString;
    }

    static void createCommandLineTemplate(std::stringstream& out, CLI::App* app, utils::CallForCommandline::Mode mode);

    static std::string createCommandLineSubcommands(CLI::App* app, utils::CallForCommandline::Mode mode) {
        std::stringstream out;

        const CLI::Option* disabledOpt = app->get_option_no_throw("--disabled");
        if (disabledOpt == nullptr || !disabledOpt->as<bool>() || mode == utils::CallForCommandline::Mode::COMPLETE) {
            for (CLI::App* subcommand : app->get_subcommands({})) {
                if (!subcommand->get_name().empty()) {
                    createCommandLineTemplate(out, subcommand, mode);
                }
            }
        }

        return out.str();
    }

    static void createCommandLineTemplate(std::stringstream& out, CLI::App* app, utils::CallForCommandline::Mode mode) {
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

    static std::string createCommandLineTemplate(CLI::App* app, utils::CallForCommandline::Mode mode) {
        std::stringstream out;

        createCommandLineTemplate(out, app, mode);

        std::string outString = out.str();
        while (app->get_parent() != nullptr) {
            app = app->get_parent();
            std::string parentOptions = createCommandLineOptions(app, mode);
            outString =
                std::string(app->get_name()).append(" ").append(!parentOptions.empty() ? parentOptions.append(" ") : "").append(outString);
        }

        if (outString.empty()) {
            outString = Config::getApplicationName();
        }

        return outString;
    }

    static std::string getCommandLine(CLI::App* commandlineTriggerApp) {
        const std::string modeString = commandlineTriggerApp->get_option("--command-line")->as<std::string>();

        utils::CallForCommandline::Mode mode = utils::CallForCommandline::Mode::STANDARD;
        std::ostringstream out;

        if (modeString == "standard") {
            out << "Below is a command line viewing all non-default and required options:\n"
                   "* Options show their configured value\n"
                   "* Required but not yet configured options show <REQUIRED> as value\n"
                   "* Options marked as <REQUIRED> need to be configured for a successful bootstrap"
                << std::endl;
            mode = utils::CallForCommandline::Mode::STANDARD;

        } else if (modeString == "active") {
            out << "Below is a command line viewing the active set of options with their default or configured values:\n"
                   "* Options show either their configured or default value\n"
                   "* Required but not yet configured options show <REQUIRED> as value\n"
                   "* Options marked as <REQUIRED> need to be configured for a successful bootstrap"
                << std::endl;
            mode = utils::CallForCommandline::Mode::ACTIVE;
        } else if (modeString == "complete") {
            out << "Below is a command line viewing the complete set of options with their default values\n"
                   "* Options show their default value\n"
                   "* Required but not yet configured options show <REQUIRED> as value\n"
                   "* Options marked as <REQUIRED> need to be configured for a successful bootstrap"
                << std::endl;
            mode = utils::CallForCommandline::Mode::COMPLETE;
        } else if (modeString == "required") {
            out << "Below is a command line viewing required options only:\n"
                   "* Options show either their configured or default value\n"
                   "* Required but not yet configured options show <REQUIRED> as value\n"
                   "* Options marked as <REQUIRED> need to be configured for a successful bootstrap"
                << std::endl;
            mode = utils::CallForCommandline::Mode::REQUIRED;
        }

        out << std::endl
            << Color::Code::FG_GREEN << "command@line" << Color::Code::FG_DEFAULT << ":" << Color::Code::FG_BLUE << "~/> "
            << Color::Code::FG_DEFAULT << createCommandLineTemplate(commandlineTriggerApp, mode) << std::endl
            << std::endl;

        return out.str();
    }

    static std::string getConfig(CLI::App* configTriggeredApp) {
        std::stringstream out;

        try {
            out << configTriggeredApp->config_to_str(true, true);
        } catch (const CLI::ParseError& e) {
            out << std::string{"["} << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT
                << "] Showing current config: " << configTriggeredApp->get_name() << " " << e.get_name() << " " << e.what();
        }

        return out.str();
    }

    static std::string doWriteConfig(CLI::App* app) {
        std::stringstream out;

        std::ofstream confFile((*app)["--write-config"]->as<std::string>());
        if (confFile.is_open()) {
            try {
                confFile << app->config_to_str(true, true);
                confFile.close();
                out << std::string{"["} << Color::Code::FG_GREEN << "SUCCESS" << Color::Code::FG_DEFAULT
                    << "] Writing config file: " + app->get_option_no_throw("--write-config")->as<std::string>() << std::endl
                    << std::endl;
            } catch (const CLI::ParseError& e) {
                confFile.close();
                out << std::string{"["} << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT
                    << "] Writing config file: " << e.get_name() << " " << e.what() << std::endl;
            }
            confFile.close();
        } else {
            out << std::string{"["} << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT
                << "] Writing config file: " << std::strerror(errno) << std::endl;
        }

        return out.str();
    }

    static std::string getHelp(CLI::App* app, CLI::App* helpTriggerApp) {
        std::stringstream out;

        const std::string helpMode = (helpTriggerApp != nullptr ? helpTriggerApp : app)->get_option("--help")->as<std::string>();

        const CLI::App* helpApp = nullptr;
        CLI::AppFormatMode mode = CLI::AppFormatMode::Normal;
        if (helpMode == "exact") {
            helpApp = helpTriggerApp;
        } else if (helpMode == "expanded") {
            helpApp = helpTriggerApp;
            mode = CLI::AppFormatMode::All;
        }
        try {
            out << app->help(helpApp, "", mode);
        } catch (CLI::ParseError& e) {
            out << std::string{"["} << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT << "] Show help: " << e.get_name() << " "
                << e.what();
        }

        return out.str();
    }

    bool Config::parse1() {
        bool proceed = parse2(true);

        if (proceed) {
            if (app->get_option_no_throw("--kill")->count() > 0) {
                try {
                    const pid_t daemonPid = utils::Daemon::stopDaemon(pidDirectory + "/" + applicationName + ".pid");
                    std::cout << "Daemon terminated: Pid = " << daemonPid << std::endl;
                } catch (const DaemonError& e) {
                    std::cout << "DaemonError: " << e.what() << std::endl;
                } catch (const DaemonFailure& e) {
                    std::cout << "DaemonFailure: " << e.what() << std::endl;
                }

                proceed = false;
            } else {
                if (helpTriggerApp == nullptr && showConfigTriggerApp == nullptr && commandlineTriggerApp == nullptr &&
                    versionOpt->count() == 0) {
                    logger::Logger::setLogLevel(logLevelOpt->as<int>());
                    logger::Logger::setVerboseLevel(verboseLevelOpt->as<int>());
                }

                logger::Logger::setQuiet(quietOpt->as<bool>());

                app->allow_extras(false);
            }
        }

        return proceed;
    }

    bool Config::parse2(bool parse1) {
        bool proceed = false;

        try {
            try {
                try {
                    helpTriggerApp = nullptr;
                    showConfigTriggerApp = nullptr;
                    commandlineTriggerApp = nullptr;

                    app->parse(argc, argv);

                    if (!parse1) {
                        if (showConfigTriggerApp != nullptr) {
                            std::cout << getConfig(showConfigTriggerApp);
                        } else if (commandlineTriggerApp != nullptr) {
                            std::cout << getCommandLine(commandlineTriggerApp);
                        } else if (app->get_option("--write-config")->count() > 0) {
                            std::cout << doWriteConfig(app.get());
                        } else {
                            proceed = true;
                        }
                    } else {
                        proceed = true;
                    }
                } catch (const CLI::Success&) {
                    if (!parse1) {
                        throw;
                    }

                    proceed = true;
                } catch (const CLI::ParseError& e) {
                    if (helpTriggerApp != nullptr || showConfigTriggerApp != nullptr || commandlineTriggerApp != nullptr ||
                        versionOpt->count() > 0) {
                        std::cout << std::string{"["} << Color::Code::FG_RED << e.get_name() << Color::Code::FG_DEFAULT << "] " << e.what()
                                  << std::endl
                                  << std::endl;

                        if (versionOpt->count() > 0) {
                            throw CLI::CallForVersion();
                        }
                        if (helpTriggerApp != nullptr) {
                            throw CLI::CallForHelp();
                        }
                        if (showConfigTriggerApp != nullptr) {
                            std::cout << getConfig(showConfigTriggerApp);
                        } else if (commandlineTriggerApp != nullptr) {
                            std::cout << getCommandLine(commandlineTriggerApp);
                        }
                    } else {
                        logger::Logger::setQuiet();
                        throw;
                    }
                }
            } catch (const DaemonError& e) {
                std::cout << std::string{"["} << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT << "] Daemonize: " << e.what()
                          << " ... exiting" << std::endl;
            } catch (const DaemonFailure& e) {
                std::cout << std::string{"["} << Color::Code::FG_RED << "Failure" << Color::Code::FG_DEFAULT << "] Daemonize: " << e.what()
                          << " ... exiting" << std::endl;
            } catch (const DaemonSignaled& e) {
                std::cout << "Pid: " << getpid() << ", child pid: " << e.getPid() << ": " << e.what() << std::endl;
            } catch (const CLI::CallForHelp&) {
                std::cout << getHelp(app.get(), helpTriggerApp);
            } catch (const CLI::CallForVersion&) {
                std::cout << app->version() << std::endl << std::endl;
            } catch (const CLI::ConversionError& e) {
                std::cout << std::string{"["} << Color::Code::FG_RED << e.get_name() << Color::Code::FG_DEFAULT << "] " << e.what()
                          << std::endl;
                throw;
            } catch (const CLI::ArgumentMismatch& e) {
                std::cout << std::string{"["} << Color::Code::FG_RED << e.get_name() << Color::Code::FG_DEFAULT << "] " << e.what()
                          << std::endl;
                throw;
            } catch (const CLI::ConfigError& e) {
                std::cout << std::string{"["} << Color::Code::FG_RED << e.get_name() << Color::Code::FG_DEFAULT << "] " << e.what()
                          << std::endl;
                std::cout << "              Adding '-w' on the command line may solve this problem" << std::endl;
                throw;
            } catch (const CLI::ParseError& e) {
                const std::string what = e.what();
                if (what.find("[Option Group: ") != std::string::npos) { // If CLI11 throws that error it means for us there are
                                                                         // unconfigured anonymous instances
                    std::cout << Color::Code::FG_RED << e.get_name() << Color::Code::FG_DEFAULT
                              << " Anonymous instance(s) not configured in source code " << std::endl;
                } else {
                    std::cout << std::string{"["} << Color::Code::FG_RED << e.get_name() << Color::Code::FG_DEFAULT << "] " << what
                              << std::endl;
                }
                throw;
            }
        } catch ([[maybe_unused]] const CLI::ParseError& e) {
            std::cout << std::endl << "Append -h or --help to your command line for more information." << std::endl;
        } catch (const CLI::Error& e) {
            std::cout << std::string{"["} << Color::Code::FG_RED << e.get_name() << Color::Code::FG_DEFAULT << "] " << e.what()
                      << std::endl;

            std::cout << std::endl << "Append -h or --help to your command line for more information." << std::endl;
        }

        return proceed;
    }

    //////////////////

    static std::shared_ptr<CLI::HelpFormatter> makeSectionFormatter() {
        const std::shared_ptr<CLI::HelpFormatter> sectionFormatter = std::make_shared<CLI::HelpFormatter>();

        sectionFormatter->label("SUBCOMMAND", "SECTION");
        sectionFormatter->label("SUBCOMMANDS", "SECTIONS");
        sectionFormatter->label("PERSISTENT", "");
        sectionFormatter->label("Persistent Options", "Options (persistent)");
        sectionFormatter->label("Nonpersistent Options", "Options (nonpersistent)");
        sectionFormatter->label("Usage", "\nUsage");
        sectionFormatter->label("bool:{true,false}", "{true,false}");
        sectionFormatter->label(":{standard,active,complete,required}", "{standard,active,complete,required}");
        sectionFormatter->label(":{standard,exact,expanded}", "{standard,exact,expanded}");
        sectionFormatter->column_width(7);

        return sectionFormatter;
    }

    CLI::App* Config::addStandardFlags(CLI::App* app) {
        app //
            ->add_flag_function(
                "-s,--show-config",
                [app](std::size_t) {
                    if (showConfigTriggerApp == nullptr) {
                        showConfigTriggerApp = app;
                    }
                },
                "Show current configuration and exit") //
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        app //
            ->add_flag(
                "--command-line{standard}",
                [app]([[maybe_unused]] std::int64_t count) {
                    if (commandlineTriggerApp == nullptr) {
                        commandlineTriggerApp = app;
                    }
                },
                "Print command-line\n"
                "* standard (default): Show all non-default and required options\n"
                "* active: Show all active options\n"
                "* complete: Show the complete option set with default values\n"
                "* required: Show only required options")
            ->configurable(false)
            ->take_first()
            ->check(CLI::IsMember({"standard", "active", "complete", "required"}))
            ->trigger_on_parse();

        return app;
    }

    CLI::App* Config::addSimpleHelp(CLI::App* app) {
        app->set_help_flag(
               "--help{exact},-h{exact}",
               [app](std::size_t) {
                   if (helpTriggerApp == nullptr) {
                       helpTriggerApp = app;
                   }
               },
               "Print help message and exit\n"
               "* standard: display help for the last command processed\n"
               "* exact: display help for the command directly preceding --help")
            ->group(app->get_formatter()->get_label("Nonpersistent Options"))
            ->take_first()
            ->check(CLI::IsMember({"standard", "exact"}))
            ->trigger_on_parse();

        return app;
    }

    CLI::App* Config::addHelp(CLI::App* app) {
        app->set_help_flag(
               "-h{exact},--help{exact}",
               [app](std::size_t) {
                   if (helpTriggerApp == nullptr) {
                       helpTriggerApp = app;
                   }
               },
               "Print help message and exit\n"
               "* standard: display help for the last command processed\n"
               "* exact: display help for the command directly preceding --help\n"
               "* expanded: print help including all descendant command options")
            ->group(app->get_formatter()->get_label("Nonpersistent Options"))
            ->take_first()
            ->check(CLI::IsMember({"standard", "exact", "expanded"}))
            ->trigger_on_parse();

        return app;
    }

    //////////////////

    std::shared_ptr<CLI::Formatter> Config::sectionFormatter = makeSectionFormatter();

    CLI::App* Config::newInstance(std::shared_ptr<CLI::App> appWithPtr, const std::string& group, bool final) {
        CLI::App* instanceSc = app->add_subcommand(appWithPtr)
                                   ->group(group)
                                   ->ignore_case(false)
                                   ->fallthrough()
                                   ->formatter(sectionFormatter)
                                   ->configurable(false)
                                   ->config_formatter(app->get_config_formatter())
                                   ->allow_extras()
                                   ->disabled(appWithPtr->get_name().empty());

        instanceSc //
            ->option_defaults()
            ->configurable(!instanceSc->get_disabled());

        if (!instanceSc->get_disabled()) {
            if (aliases.contains(instanceSc->get_name())) {
                instanceSc //
                    ->alias(aliases[instanceSc->get_name()]);
            }
        }

        utils::Config::addStandardFlags(instanceSc);

        if (!final) {
            utils::Config::addHelp(instanceSc);
        } else {
            utils::Config::addSimpleHelp(instanceSc);
        }

        return instanceSc;
    }

    void Config::required(CLI::App* instance, bool required) {
        if (required) {
            app->needs(instance);

            for (const auto& sub : instance->get_subcommands([](const CLI::App* sc) -> bool {
                     return sc->get_required();
                 })) {
                instance->needs(sub);
            }
        } else {
            app->remove_needs(instance);

            for (const auto& sub : instance->get_subcommands([](const CLI::App* sc) -> bool {
                     return sc->get_required();
                 })) {
                instance->remove_needs(sub);
            }
        }

        instance->required(required);
        instance->ignore_case(required);
    }

    void Config::disabled(CLI::App* instance, bool disabled) {
        if (disabled) {
            if (instance->get_ignore_case()) {
                app->remove_needs(instance);
            }

            for (const auto& sub : instance->get_subcommands({})) {
                if (sub->get_ignore_case()) {
                    instance->remove_needs(sub);
                    sub->required(false); // ### must be stored in ConfigInstance
                }
            }
        } else {
            if (instance->get_ignore_case()) {
                app->needs(instance);
            }

            for (const auto& sub : instance->get_subcommands({})) {
                if (sub->get_ignore_case()) { // ### must be recalled from ConfigInstance
                    instance->needs(sub);
                    sub->required();
                }
            }
        }

        instance->required(disabled ? false : instance->get_ignore_case());
    }

    bool Config::removeInstance(CLI::App* instance) {
        Config::required(instance, false);

        return app->remove_subcommand(instance);
    }

    int Config::argc = 0;
    char** Config::argv = nullptr;

    std::string Config::applicationName;

    std::string Config::configDirectory;
    std::string Config::logDirectory;
    std::string Config::pidDirectory;

    CLI::Option* Config::daemonizeOpt = nullptr;
    CLI::Option* Config::logFileOpt = nullptr;
    CLI::Option* Config::monochromLogOpt = nullptr;
    CLI::Option* Config::userNameOpt = nullptr;
    CLI::Option* Config::groupNameOpt = nullptr;
    CLI::Option* Config::enforceLogFileOpt = nullptr;
    CLI::Option* Config::logLevelOpt = nullptr;
    CLI::Option* Config::verboseLevelOpt = nullptr;
    CLI::Option* Config::quietOpt = nullptr;

    CLI::Option* Config::versionOpt = nullptr;

    CLI::App* Config::helpTriggerApp = nullptr;
    CLI::App* Config::showConfigTriggerApp = nullptr;
    CLI::App* Config::commandlineTriggerApp = nullptr;

    std::map<std::string, std::string> Config::aliases;
    std::map<std::string, CLI::Option*> Config::applicationOptions;

} // namespace utils
