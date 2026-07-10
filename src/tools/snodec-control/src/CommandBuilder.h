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

#ifndef SNODECCONTROL_COMMANDBUILDER_H
#define SNODECCONTROL_COMMANDBUILDER_H

#include <optional>
#include <string>
#include <vector>

namespace snodec::control {

    // Shell-escapes a single argument for human-readable display (e.g. --print-run-command). This is
    // display-only: actual process execution always goes through argv arrays via posix_spawn and never
    // a shell, so escaping here can never affect what is actually executed.
    std::string shellEscape(const std::string& argument);

    // Joins `executable` and `arguments` into a single shell-escaped, space-separated string suitable
    // for display.
    std::string formatCommandForDisplay(const std::string& executable, const std::vector<std::string>& arguments);

    // True if `arguments` already contains a `--config-file`/`-c` flag, either as its own token or as
    // `--config-file=...`/`-c=...`. Used to detect a conflict before snodec-control appends its own.
    bool containsConfigFileFlag(const std::vector<std::string>& arguments);

    // Builds the argv (excluding argv[0], the executable path itself) for a `target -s` discovery
    // invocation: the tokenized --target-args followed by "-s".
    std::vector<std::string> buildDiscoveryArgs(const std::vector<std::string>& targetArgTokens);

    // Builds the argv for asking the target to read `tempConfigPath` and write the canonical config to
    // `outputConfigPath`: the tokenized --target-args followed by "--config-file" <tempConfigPath>
    // "--write-config" <outputConfigPath>. Returns std::nullopt if `targetArgTokens` already specifies a
    // config file, since appending our own would conflict with it.
    std::optional<std::vector<std::string>>
    buildSaveArgs(const std::vector<std::string>& targetArgTokens, const std::string& tempConfigPath, const std::string& outputConfigPath);

    // Builds the argv for running the target: the tokenized --target-args, optionally followed by
    // "--config-file" <*configPath> when `configPath` has a value. Returns std::nullopt if
    // `targetArgTokens` already specifies a config file and `configPath` also has a value, since
    // appending our own would conflict with it.
    std::optional<std::vector<std::string>> buildRunArgs(const std::vector<std::string>& targetArgTokens,
                                                         const std::optional<std::string>& configPath);

} // namespace snodec::control

#endif // SNODECCONTROL_COMMANDBUILDER_H
