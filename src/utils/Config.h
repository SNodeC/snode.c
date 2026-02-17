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

namespace utils {

    class Config {
    public:
        static CLI::App* newInstance(std::shared_ptr<CLI::App> appWithPtr, const std::string& group, bool final = false);

        template <typename T>
        static T* addInstance();

        template <typename T>
        static T* getInstance();

        //////////////////

        Config() = delete;
        Config(const Config&) = delete;
        ~Config() = delete;

        Config& operator=(const Config&) = delete;

        static bool init(int argc, char* argv[]);
        static bool bootstrap();
        static void terminate();

        static bool removeInstance(CLI::App* instance);

        static void required(CLI::App* instance, bool required = true);
        static void disabled(CLI::App* instance, bool disabled = true);

        static CLI::App* addStandardFlags(CLI::App* app);
        static CLI::App* addSimpleHelp(CLI::App* app);
        static CLI::App* addHelp(CLI::App* app);

        static std::string getApplicationName();
        static int getLogLevel();
        static int getVerboseLevel();

    private:
        static bool parse1();

    public:
        static bool parse2();

    private:
        static int argc;
        static char** argv;

        static std::shared_ptr<CLI::App> app;

        static bool subParse;
        static std::string applicationName;

        static std::string configDirectory;
        static std::string logDirectory;
        static std::string pidDirectory;

        static CLI::Option* daemonizeOpt;
        static CLI::Option* logFileOpt;
        static CLI::Option* userNameOpt;
        static CLI::Option* groupNameOpt;
        static CLI::Option* enforceLogFileOpt;
        static CLI::Option* logLevelOpt;
        static CLI::Option* verboseLevelOpt;
        static CLI::Option* quietOpt;

    public:
        static CLI::App* helpTriggerApp;
        static CLI::App* showConfigTriggerApp;

    private:
        static std::shared_ptr<CLI::Formatter> sectionFormatter;

        static std::map<std::string, std::string> aliases;             // from -> to
        static std::map<std::string, CLI::Option*> applicationOptions; // keep all user options in memory

        static std::vector<std::shared_ptr<void>> configSections; // Store anything
    };

    //////////////////
} // namespace utils

#endif // UTILS_CONFIG_H
