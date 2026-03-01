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

#ifndef UTILS_CONFIG_H
#define UTILS_CONFIG_H

#include "utils/SubCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
    class Formatter;
} // namespace CLI

#include <map>
#include <memory>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils::config {

    class ConfigRoot : public utils::SubCommand {
    public:
        ConfigRoot();

        ConfigRoot* addRootOptions(const std::string& applicationName,
                                   const std::string& userName,
                                   const std::string& groupName,
                                   const std::string& configDirectory,
                                   const std::string& logDirectory,
                                   const std::string& pidDirectory);

        bool parse1(int argc, char* argv[]);
        bool bootstrap(int argc, char* argv[]);
        void terminate();

    protected:
        bool parse2(int argc, char* argv[], bool parse1 = false);

    private:
        std::string applicationName;
        std::string pidDirectory;

        CLI::Option* daemonizeOpt;
        CLI::Option* logFileOpt;
        CLI::Option* monochromLogOpt;
        CLI::Option* userNameOpt;
        CLI::Option* groupNameOpt;
        CLI::Option* enforceLogFileOpt;
        CLI::Option* logLevelOpt;
        CLI::Option* verboseLevelOpt;
        CLI::Option* quietOpt;

        CLI::Option* versionOpt;
    };

} // namespace utils::config

namespace utils {

    class Config {
    public:
        Config() = delete;
        Config(const Config&) = delete;
        ~Config() = delete;

        Config& operator=(const Config&) = delete;

        static bool init(int argc, char* argv[]);
        static bool bootstrap();
        static void terminate();

        static const std::string& getApplicationName();
        static int getLogLevel();
        static int getVerboseLevel();

    private:
        static bool parse1();

    public:
        static bool parse2(bool parse1 = false);

        static utils::config::ConfigRoot configRoot;

        /*
                //////////////////

                static CLI::App* addStandardFlags(CLI::App* app);
                static CLI::App* addSimpleHelp(CLI::App* app);
                static CLI::App* addHelp(CLI::App* app);

                //////////////////

                static CLI::App* newInstance(std::shared_ptr<CLI::App> appWithPtr, const std::string& group, bool final = false);

                static void required(CLI::App* instance, bool required = true);
                static void disabled(CLI::App* instance, bool disabled = true);

                static bool removeInstance(CLI::App* instance);

                template <typename T>
                static T* addInstance();

                template <typename T>
                static T* getInstance();

                //////////////////
        */
    private:
        static int argc;
        static char** argv;

        //        static std::shared_ptr<CLI::App> app;

        static bool subParse;
        static std::string applicationName;

        static std::string configDirectory;
        static std::string logDirectory;
        static std::string pidDirectory;

    public:
        static CLI::App* helpTriggerApp;
        static CLI::App* showConfigTriggerApp;
        static CLI::App* commandlineTriggerApp;
    };

    //////////////////
} // namespace utils

#endif // UTILS_CONFIG_H
