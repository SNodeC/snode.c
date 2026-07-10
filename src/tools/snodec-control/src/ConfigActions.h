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

#ifndef SNODECCONTROL_CONFIGACTIONS_H
#define SNODECCONTROL_CONFIGACTIONS_H

#include "ConfigEditor.h"
#include "ConfigModel.h"
#include "ConfigParser.h"
#include "Metadata.h"

#include <optional>
#include <string>
#include <vector>

namespace snodec::control {

    // The shared, presentation-agnostic "business logic" behind every snodec-control action: running
    // discovery, formatting inspection output, and saving/resolving a run configuration through the
    // target. Both the plain CLI (Cli.cpp) and the interactive UI (ui/CursesUi.cpp) call these same
    // functions, so behavior (including SNode.C's non-zero-exit-on-success quirk for `-s`/`-w`) is
    // defined exactly once. None of these functions touch curses or any UI presentation concern; they
    // only return data/strings for the caller to display however is appropriate.

    struct DiscoveryOutcome {
        bool ok = false;
        std::string fatalError;   // set only when ok == false; ready to print as-is
        std::string diagnostics;  // informational notes/warnings to print regardless of ok (may be empty)
        std::string rawStdOut;    // the target's raw '-s' stdout, for --write-template
        ParseResult parseResult;  // valid when ok == true
        ParsedMetadata metadata;  // valid when ok == true; metadata.usable() decides metadata-native vs legacy hierarchy
    };

    // Runs `<targetPath> <targetArgTokens...> -s`, capturing and parsing its output. `diagnostics`
    // always reflects the same non-zero-exit-code notes and parser warnings Phase 1/2 always printed;
    // the caller decides where/whether to display them (immediately via stderr for the plain CLI, or
    // buffered until curses is up for the UI's own use).
    DiscoveryOutcome discoverTarget(const std::string& targetPath, const std::vector<std::string>& targetArgTokens);

    std::string formatSummary(const ConfigModel& model, const std::string& targetPath);
    std::string formatOptionBlock(const ConfigOption& option);
    std::string formatListOptions(const ConfigModel& model);
    std::string formatDiff(const std::vector<ChangeRecord>& changes);
    std::string formatMissingRequired(const std::vector<const ConfigOption*>& missing);

    // Writes `content` to `path`, creating parent directories if needed. Returns false with `error` set
    // on failure.
    bool writeFile(const std::string& path, const std::string& content, std::string& error);

    // Creates a uniquely named temporary file containing `content` and returns its path, or an empty
    // string (with `error` set) on failure. The caller is responsible for removing the file again.
    std::string createTempFile(const std::string& content, std::string& error);

    struct RunConfigResolution {
        bool ok = true;
        std::string error;
        std::optional<std::string> configPath;
        std::string tempPathCreated; // non-empty only if a temp file was actually created (needs cleanup)
    };

    // Resolves the config file (if any) to use for --run / --print-run-command, in priority order: an
    // explicit `runConfigPath`, then `savedConfigPathForRun` (a config file just written by
    // --save-config in this same invocation), then (if `haveEdits`) a temporary materialized config,
    // then no config file at all. Under `dryRun`, no temporary file is actually created; a placeholder
    // path is returned purely for display purposes.
    RunConfigResolution resolveRunConfigPath(const std::optional<std::string>& runConfigPath,
                                              const std::optional<std::string>& saveConfigPath,
                                              const std::optional<std::string>& savedConfigPathForRun,
                                              bool haveEdits,
                                              bool dryRun,
                                              const ConfigModel& model,
                                              const std::string& targetPath);

    struct SaveOutcome {
        bool succeeded = false;
        bool isError = false;
        std::string message; // ready to print as-is (already includes "Error:"/"Note:"/"[dry-run]" etc.)
    };

    // Materializes `model` to a temporary file and asks the target to read it and write the canonical
    // config to `saveConfigPath` (via --config-file/--write-config), exactly as Phase 2 documented.
    SaveOutcome performSaveConfig(const ConfigModel& model,
                                  const std::string& targetPath,
                                  const std::vector<std::string>& targetArgTokens,
                                  const std::string& saveConfigPath,
                                  bool dryRun,
                                  bool keepTemp);

} // namespace snodec::control

#endif // SNODECCONTROL_CONFIGACTIONS_H
