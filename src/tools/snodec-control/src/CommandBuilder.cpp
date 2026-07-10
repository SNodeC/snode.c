/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
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

#include "CommandBuilder.h"

#include <algorithm>
#include <cctype>

namespace snodec::control {

    namespace {

        bool isSafeUnquoted(const std::string& value) {
            if (value.empty()) {
                return false;
            }

            return std::all_of(value.begin(), value.end(), [](char character) {
                return std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '.' || character == '-' ||
                       character == '_' || character == '/' || character == ':' || character == '=' || character == ',' || character == '+';
            });
        }

    } // namespace

    std::string shellEscape(const std::string& argument) {
        if (isSafeUnquoted(argument)) {
            return argument;
        }

        std::string escaped = "'";
        for (const char character : argument) {
            if (character == '\'') {
                escaped += "'\\''";
            } else {
                escaped.push_back(character);
            }
        }
        escaped.push_back('\'');

        return escaped;
    }

    std::string formatCommandForDisplay(const std::string& executable, const std::vector<std::string>& arguments) {
        std::string command = shellEscape(executable);

        for (const std::string& argument : arguments) {
            command += " " + shellEscape(argument);
        }

        return command;
    }

    bool containsConfigFileFlag(const std::vector<std::string>& arguments) {
        return std::any_of(arguments.begin(), arguments.end(), [](const std::string& argument) {
            return argument == "--config-file" || argument == "-c" || argument.rfind("--config-file=", 0) == 0 ||
                   argument.rfind("-c=", 0) == 0;
        });
    }

    std::vector<std::string> buildDiscoveryArgs(const std::vector<std::string>& targetArgTokens) {
        std::vector<std::string> args = targetArgTokens;
        args.emplace_back("-s");

        return args;
    }

    std::optional<std::vector<std::string>>
    buildSaveArgs(const std::vector<std::string>& targetArgTokens, const std::string& tempConfigPath, const std::string& outputConfigPath) {
        if (containsConfigFileFlag(targetArgTokens)) {
            return std::nullopt;
        }

        std::vector<std::string> args = targetArgTokens;
        args.emplace_back("--config-file");
        args.push_back(tempConfigPath);
        args.emplace_back("--write-config");
        args.push_back(outputConfigPath);

        return args;
    }

    std::optional<std::vector<std::string>> buildRunArgs(const std::vector<std::string>& targetArgTokens,
                                                         const std::optional<std::string>& configPath) {
        std::vector<std::string> args = targetArgTokens;

        if (configPath.has_value()) {
            if (containsConfigFileFlag(targetArgTokens)) {
                return std::nullopt;
            }
            args.emplace_back("--config-file");
            args.push_back(*configPath);
        }

        return args;
    }

} // namespace snodec::control
