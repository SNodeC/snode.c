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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <grp.h>
#include <iostream>
#include <map>
#include <memory>
#include <pwd.h>
#include <sstream>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define XSTR(s) STR(s)
#define STR(s) #s

namespace utils::CallForCommandline {
    enum class Mode { REQUIRED, STANDARD, ACTIVE, COMPLETE };
}

namespace utils {

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

    static std::string doWriteConfig(utils::SubCommand* subCommand) {
        std::stringstream out;

        std::ofstream confFile(subCommand->getOption("--write-config")->as<std::string>());
        if (confFile.is_open()) {
            try {
                confFile << subCommand->configToStr();
                confFile.close();
                out << std::string{"["} << Color::Code::FG_GREEN << "SUCCESS" << Color::Code::FG_DEFAULT
                    << "] Writing config file: " + subCommand->getOption("--write-config")->as<std::string>() << std::endl
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

    static std::string getHelp(utils::SubCommand* subCommand, CLI::App* helpTriggerApp) {
        std::stringstream out;

        const std::string helpMode =
            (helpTriggerApp != nullptr ? helpTriggerApp->get_option("--help") : subCommand->getOption("--help"))->as<std::string>();

        const CLI::App* helpApp = nullptr;
        CLI::AppFormatMode mode = CLI::AppFormatMode::Normal;
        if (helpMode == "exact") {
            helpApp = helpTriggerApp;
        } else if (helpMode == "expanded") {
            helpApp = helpTriggerApp;
            mode = CLI::AppFormatMode::All;
        }
        try {
            out << subCommand->help(helpApp, mode);
        } catch (CLI::ParseError& e) {
            out << std::string{"["} << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT << "] Show help: " << e.get_name() << " "
                << e.what();
        }

        return out.str();
    }

    ConfigRoot::ConfigRoot()
        : utils::SubCommand(nullptr, std::make_shared<utils::AppWithPtr>("Root of config", "", this), "", false) {
        logger::Logger::init();
    }

    ConfigRoot::~ConfigRoot() {
    }

    ConfigRoot* ConfigRoot::addRootOptions(const std::string& applicationName,
                                           const std::string& userName,
                                           const std::string& groupName,
                                           const std::string& configDirectory,
                                           const std::string& logDirectory,
                                           const std::string& pidDirectory) {
        this->applicationName = applicationName;
        this->pidDirectory = pidDirectory;

        description("Configuration for Application '" + applicationName + "'");

        footer("Application '" + applicationName +
               "' powered by SNode.C\n"
               "(C) 2020-2026 Volker Christian <me@vchrist.at>\n"
               "https://github.com/SNodeC/snode.c");

        addConfigFlag(configDirectory + "/" + applicationName + ".conf");

        writeConfigOpt = setConfigurable(addOption( //
                                             "-w,--write-config",
                                             "Write config file and exit",
                                             "configfile",
                                             configDirectory + "/" + applicationName + ".conf",
                                             !CLI::ExistingDirectory),
                                         false)
                             ->expected(0, 1);

        killOpt = setConfigurable(addFlag("-k,--kill", "Kill running daemon", "", CLI::Validator()), false);

        logLevelOpt = setConfigurable(addOption("--log-level", "Log level", "level", 4, CLI::Range(0, 6)), true);

        verboseLevelOpt = setConfigurable(addOption("--verbose-level", "Verbose level", "level", 2, CLI::Range(0, 10)), true);

        logFileOpt = setConfigurable(addLogFileFlag(logDirectory + "/" + applicationName + ".log"), true);

        monochromLogOpt = setConfigurable(addFlagFunction( //
                                              "-m{true},--monochrom-logmonochromLogOption{true}",
                                              [&monochromLogOpt = this->monochromLogOpt]() {
                                                  if (monochromLogOpt->as<bool>()) {
                                                      logger::Logger::setDisableColor(true);
                                                  } else {
                                                      logger::Logger::setDisableColor(false);
                                                  }
                                              },
                                              "Monochrom log output",
                                              "BOOL",
                                              logger::Logger::getDisableColor() ? "true" : "false",
                                              CLI::IsMember({"true", "false"})),
                                          true)
                              ->trigger_on_parse();

        quietOpt = setConfigurable(addFlag( //
                                       "-q{true},--quiet{true}",
                                       "Quiet mode",
                                       "BOOL",
                                       "false",
                                       CLI::IsMember({"true", "false"})),
                                   true);

        enforceLogFileOpt = setConfigurable(addFlag( //
                                                "-e{true},--enforce-log-file{true}",
                                                "Enforce writing of logs to file for foreground applications",
                                                "BOOL",
                                                "false",
                                                CLI::IsMember({"true", "false"})),
                                            true);

        daemonizeOpt = setConfigurable(addFlag( //
                                           "-d{true},--daemonize{true}",
                                           "Start application as daemon",
                                           "BOOL",
                                           "false",
                                           CLI::IsMember({"true", "false"})),
                                       true);

        userNameOpt = setConfigurable(addOption( //
                                          "-u,--user-name",
                                          "Run daemon under specific user permissions",
                                          "username",
                                          userName,
                                          CLI::TypeValidator<std::string>()),
                                      true);

        groupNameOpt = setConfigurable(addOption( //
                                           "-g,--group-name",
                                           "Run daemon under specific group permissions",
                                           "groupname",
                                           groupName,
                                           CLI::TypeValidator<std::string>()),
                                       true);

        aliasOpt = setConfigurable(addOption("-a,--aliases",
                                             "Make an instance also known as an alias in configuration files",
                                             "instance=instance_alias [instance=instance_alias [...]]",
                                             CLI::TypeValidator<std::string>()),
                                   false);

        versionOpt = addVersionFlag("1.0-rc1");

        return this;
    }

    bool ConfigRoot::parse1(int argc, char* argv[]) {
        allowExtras(true);

        helpOpt->get_validator()->active(false);

        bool proceed = parse2(argc, argv, true);

        helpOpt->get_validator()->active(true);

        if (proceed) {
            if (killOpt->count() > 0) {
                try {
                    const pid_t daemonPid =
                        utils::Daemon::stopDaemon(pidDirectory + "/" + std::string(std::filesystem::path(argv[0]).filename()) + ".pid");
                    std::cout << "Daemon terminated: Pid = " << daemonPid << std::endl;
                } catch (const DaemonError& e) {
                    std::cout << "DaemonError: " << e.what() << std::endl;
                } catch (const DaemonFailure& e) {
                    std::cout << "DaemonFailure: " << e.what() << std::endl;
                }

                proceed = false;
            } else {
                if (helpTriggerApp == nullptr && showConfigTriggerApp == nullptr && commandlineTriggerApp == nullptr &&
                    versionOpt->count() == 0 && writeConfigOpt->count() == 0) {
                    logger::Logger::setLogLevel(logLevelOpt->as<int>());
                    logger::Logger::setVerboseLevel(verboseLevelOpt->as<int>());
                }

                logger::Logger::setQuiet(quietOpt->as<bool>());

                allowExtras(false);
            }
        }

        return proceed;
    }

    bool ConfigRoot::bootstrap(int argc, char* argv[]) {
        finalCallback([this]() {
            if (daemonizeOpt->as<bool>() && helpTriggerApp == nullptr && showConfigTriggerApp == nullptr && writeConfigOpt->count() == 0 &&
                commandlineTriggerApp == nullptr) {
                std::cout << "Running as daemon (double fork)" << std::endl;

                utils::Daemon::startDaemon(
                    pidDirectory + "/" + applicationName + ".pid", userNameOpt->as<std::string>(), groupNameOpt->as<std::string>());

                logger::Logger::setQuiet();

                const std::string logFile = logFileOpt->as<std::string>();
                if (!logFile.empty()) {
                    logger::Logger::logToFile(logFile);
                }
            } else if (enforceLogFileOpt->as<bool>()) {
                const std::string logFile = logFileOpt->as<std::string>();
                if (!logFile.empty()) {
                    std::cout << "Writing logs to file " << logFile << std::endl;

                    logger::Logger::logToFile(logFile);
                }
            }
        });

        bool proceed = parse2(argc, argv);

        return proceed;
    }

    void ConfigRoot::terminate() {
        if (daemonizeOpt->as<bool>()) {
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

        removeSubCommand();
    }

    bool ConfigRoot::parse2(int argc, char* argv[], bool parse1) {
        bool proceed = false;

        try {
            try {
                try {
                    aliases.clear();

                    helpTriggerApp = nullptr;
                    showConfigTriggerApp = nullptr;
                    commandlineTriggerApp = nullptr;

                    parse(argc, argv);

                    if (!parse1) {
                        if (showConfigTriggerApp != nullptr) {
                            std::cout << getConfig(showConfigTriggerApp);
                        } else if (commandlineTriggerApp != nullptr) {
                            std::cout << getCommandLine(commandlineTriggerApp);
                        } else if (getOption("--write-config")->count() > 0) {
                            std::cout << doWriteConfig(this);
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
                std::cout << getHelp(this, helpTriggerApp);
            } catch (const CLI::CallForVersion&) {
                std::cout << version() << std::endl << std::endl;
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
            configRoot.addRootOptions(applicationName, pw->pw_name, gr->gr_name, configDirectory, logDirectory, pidDirectory);

            proceed = configRoot.parse1(argc, argv);
        }

        return proceed;
    }

    bool Config::bootstrap() {
        return configRoot.bootstrap(argc, argv);
    }

    void Config::parse() {
        configRoot.parse(argc, argv);
    }

    void Config::terminate() {
        configRoot.terminate();
    }

    const std::string& Config::getApplicationName() {
        return applicationName;
    }

    int Config::getLogLevel() {
        return configRoot.logLevelOpt->as<int>();
    }

    int Config::getVerboseLevel() {
        return configRoot.verboseLevelOpt->as<int>();
    }

    int Config::argc = 0;
    char** Config::argv = nullptr;

    ConfigRoot Config::configRoot;

    std::string Config::applicationName;

    std::string Config::configDirectory;
    std::string Config::logDirectory;
    std::string Config::pidDirectory;

} // namespace utils
