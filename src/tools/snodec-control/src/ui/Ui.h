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

#ifndef SNODECCONTROL_UI_UI_H
#define SNODECCONTROL_UI_UI_H

#include "../ConfigEditor.h"
#include "../ConfigModel.h"
#include "../Metadata.h"

#include <optional>
#include <string>
#include <vector>

namespace snodec::control::ui {

    // The single entry point Cli.cpp needs to know about. Whether this returns true depends on whether
    // this binary was built with SNODEC_CONTROL_BUILD_TUI and Curses was found: exactly one of
    // CursesUi.cpp or UiUnavailable.cpp is compiled into the final executable, and each provides its own
    // definition of isUiAvailable()/runInteractiveUi() accordingly. Cli.cpp itself stays unaware of which.
    bool isUiAvailable();

    // Everything the UI needs from the CLI's already-parsed options and already-completed discovery, so
    // it can call the same ConfigActions functions the CLI uses for save/run without depending on the Cli
    // option-parsing internals.
    struct UiOptions {
        std::string targetPath;
        std::vector<std::string> targetArgTokens;
        std::optional<std::string> initialSaveConfigPath; // pre-set --save-config path, offered as the UI's default Save target
        bool dryRun = false;
        bool keepTemp = false;

        // Changes already applied (from --set/--unset) before the UI opened, so the UI's own Diff action
        // can preview the full end-to-end diff (pre-UI + in-UI) exactly as the final --diff would show it.
        std::vector<ChangeRecord> priorChanges;

        // From the same discovery run's DiscoveryOutcome::metadata. Default-constructed (present ==
        // false) is a legitimate value, meaning "build the tree with the legacy dotted-name heuristic",
        // for a target that predates the "#@ snodec.meta ..." comment-metadata format.
        ParsedMetadata metadata;
    };

    struct UiResult {
        bool ok = true;     // false only if the UI could not run at all; see `message`
        std::string message; // ready-to-print diagnostic: set when !ok, or for informational notes on exit

        // Net changes made while inside the UI (already excludes anything the user discarded on quit).
        // The caller merges these with any pre-UI --set/--unset changes (via mergeChangeRecords) to
        // produce one coherent --diff for the whole invocation.
        std::vector<ChangeRecord> changes;

        // Set only if the UI's own Save action successfully wrote uiOptions.initialSaveConfigPath (or a
        // path the user was prompted for), for use by --run's config-file resolution afterwards.
        std::optional<std::string> savedConfigPathForRun;
    };

    // Runs the interactive hierarchical curses UI against `model`, editing it in place until the user
    // quits. If isUiAvailable() is false, returns immediately with ok == false and an explanatory
    // message ("Interactive UI support was not built because Curses was not found."); `model` is left
    // untouched in that case.
    UiResult runInteractiveUi(ConfigModel& model, const UiOptions& uiOptions);

} // namespace snodec::control::ui

#endif // SNODECCONTROL_UI_UI_H
