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

// The real, Curses-backed implementation of src/ui/Ui.h, built only when SNODEC_CONTROL_BUILD_TUI is on
// and Curses was found (see src/CMakeLists.txt). Deliberately thin on business logic: every save/run/
// materialize/check/diff action below calls straight into ConfigActions.h/ConfigEditor.h, the same
// functions the plain CLI uses, so behavior is defined once. This file owns only rendering, input, and
// prompting.

#include "Ui.h"

#include "../CommandBuilder.h"
#include "../ConfigActions.h"
#include "../ConfigEditor.h"
#include "../Materializer.h"
#include "../ProcessRunner.h"
#include "LineEditor.h"
#include "RenderUtil.h"
#include "UiState.h"
#include "UiTree.h"

#include <curses.h>

#include <cctype>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <utility>

namespace snodec::control::ui {

    namespace {

        // RAII around initscr()/endwin() so every early return still leaves the terminal in a sane state.
        class CursesSession {
        public:
            CursesSession() {
                initscr();
                cbreak();
                noecho();
                keypad(stdscr, TRUE);
                curs_set(0);
            }

            CursesSession(const CursesSession&) = delete;
            CursesSession& operator=(const CursesSession&) = delete;

            ~CursesSession() {
                endwin();
            }
        };

        std::string containerLabel(const UiNode& node) {
            const char* bracket = node.expanded ? "[-] " : "[+] ";
            return bracket + node.label;
        }

        std::string optionLabel(const UiNode& node) {
            std::string text = node.label + " = " + effectiveValueDisplay(*node.option);
            if (isOptionMissingRequired(*node.option)) {
                text = "! " + text;
            }
            return text;
        }

        std::string nodeLineLabel(const UiNode& node) {
            return node.type == UiNodeType::Option ? optionLabel(node) : containerLabel(node);
        }

        // Capitalizes a metadata "kind" string ("instance" -> "Instance") for display; the empty-kind
        // case (should not occur for a metadata-native Node/Group, but handled defensively) falls back
        // to a generic label instead of showing nothing.
        std::string capitalize(const std::string& text) {
            if (text.empty()) {
                return text;
            }
            std::string result = text;
            result.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(result.front())));
            return result;
        }

        std::string describeContainer(const UiNode& node) {
            std::ostringstream out;
            switch (node.type) {
                case UiNodeType::ApplicationOptions:
                    out << "Application Options: " << node.children.size() << " option(s)\n";
                    break;
                case UiNodeType::InstancesRoot:
                    out << "Instances: " << node.children.size() << " instance(s)\n";
                    break;
                case UiNodeType::Instance:
                    out << "Instance: " << node.label << "\n";
                    break;
                case UiNodeType::SectionsRoot:
                    out << "Sections: " << node.children.size() << " section(s)\n";
                    break;
                case UiNodeType::Section:
                    out << "Section: " << node.label << "\n";
                    break;
                case UiNodeType::Node:
                    out << (node.kind.empty() ? "Node" : capitalize(node.kind)) << ": " << node.label;
                    if (node.disabled) {
                        out << " (disabled)";
                    }
                    out << "\n";
                    if (!node.kindSource.empty() && node.kindSource.rfind("heuristic", 0) == 0) {
                        out << "  (classification is the target's own best guess: " << node.kindSource << ")\n";
                    }
                    break;
                case UiNodeType::Group:
                    out << (node.kind.empty() ? "Group" : capitalize(node.kind) + " group") << ": " << node.label << "\n";
                    break;
                case UiNodeType::Option:
                    break;
            }
            return out.str();
        }

        void printLines(const std::string& text, int startY, int maxY, int maxX) {
            std::istringstream stream(text);
            std::string line;
            int y = startY;
            while (y < maxY - 1 && std::getline(stream, line)) {
                mvprintw(y++, 0, "%s", fitToWidth(line, maxX).c_str());
            }
        }

        // Shows `content` (may be empty) full-screen and waits for any key.
        void showMessageBox(const std::string& content, const std::string& footer = "Press any key to continue...") {
            erase();
            int maxY = 0;
            int maxX = 0;
            getmaxyx(stdscr, maxY, maxX);
            printLines(content, 0, maxY, maxX);

            attron(A_REVERSE);
            mvprintw(maxY - 1, 0, "%s", fitToWidth(footer, maxX).c_str());
            attroff(A_REVERSE);
            refresh();
            getch();
        }

        // Shows `content` full-screen and asks a yes/no `question`; Escape counts as "no".
        bool confirmWithContent(const std::string& content, const std::string& question) {
            erase();
            int maxY = 0;
            int maxX = 0;
            getmaxyx(stdscr, maxY, maxX);
            printLines(content, 0, maxY, maxX);

            attron(A_REVERSE);
            mvprintw(maxY - 1, 0, "%s", fitToWidth(question + " (y/n)", maxX).c_str());
            attroff(A_REVERSE);
            refresh();

            for (;;) {
                const int ch = getch();
                if (ch == 'y' || ch == 'Y') {
                    return true;
                }
                if (ch == 'n' || ch == 'N' || ch == 27) {
                    return false;
                }
            }
        }

        // Positions a fixed-width viewport of `content` so that `cursorPos` stays visible within `width`
        // columns, returning the visible substring and the on-screen column to place the cursor at.
        std::pair<std::string, int> scrollToCursor(const std::string& content, std::size_t cursorPos, int width) {
            if (width <= 0) {
                return {"", 0};
            }
            const std::size_t uWidth = static_cast<std::size_t>(width);
            if (content.size() < uWidth) {
                return {content, static_cast<int>(cursorPos)};
            }

            std::size_t start = 0;
            if (cursorPos + 1 > uWidth) {
                start = cursorPos + 1 - uWidth;
            }
            if (start + uWidth > content.size()) {
                start = content.size() > uWidth ? content.size() - uWidth : 0;
            }
            return {content.substr(start, uWidth), static_cast<int>(cursorPos - start)};
        }

        // Single-line text prompt on the bottom row, prefilled with `initial`, supporting cursor movement
        // (Left/Right/Home/End), Backspace/Delete, and insertion at the cursor (not just append). Enter
        // accepts (including an empty string), Escape cancels and returns std::nullopt without modifying
        // anything.
        std::optional<std::string> promptText(const std::string& label, const std::string& initial) {
            int maxY = 0;
            int maxX = 0;
            getmaxyx(stdscr, maxY, maxX);

            LineEditorBuffer editor(initial);
            curs_set(1);

            for (;;) {
                mvhline(maxY - 1, 0, ' ', maxX);
                const std::string content = label + editor.text();
                const auto [visible, cursorCol] = scrollToCursor(content, label.size() + editor.cursor(), maxX);
                mvprintw(maxY - 1, 0, "%s", fitToWidth(visible, maxX).c_str());
                move(maxY - 1, std::min(cursorCol, maxX > 0 ? maxX - 1 : 0));
                refresh();

                const int ch = getch();
                if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                    curs_set(0);
                    return editor.text();
                }
                if (ch == 27) {
                    curs_set(0);
                    return std::nullopt;
                }
                if (ch == KEY_LEFT) {
                    editor.moveLeft();
                } else if (ch == KEY_RIGHT) {
                    editor.moveRight();
                } else if (ch == KEY_HOME) {
                    editor.moveToStart();
                } else if (ch == KEY_END) {
                    editor.moveToEnd();
                } else if (ch == KEY_DC) {
                    editor.deleteForward();
                } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                    editor.backspace();
                } else if (ch >= 32 && ch < 127) {
                    editor.insert(static_cast<char>(ch));
                }
            }
        }

        // Before Save/Run: required options with no usable value never block leaving the UI on their
        // own, but the user is warned and must confirm before actually saving/running with them unmet.
        bool confirmRequiredOptionsOk(ConfigModel& model) {
            const std::vector<const ConfigOption*> missing = missingRequiredOptions(model);
            if (missing.empty()) {
                return true;
            }
            return confirmWithContent(formatMissingRequired(missing), "Some required options are still unset. Proceed anyway?");
        }

        // The path Save/Run should prefill: the path last successfully saved to in this session, else
        // the --save-config path (if any) given before --ui opened, else empty (fresh prompt).
        std::string defaultSavePath(const UiOptions& uiOptions, const std::optional<std::string>& savedConfigPathForRun) {
            return savedConfigPathForRun.value_or(uiOptions.initialSaveConfigPath.value_or(""));
        }

        void editSelectedOption(UiState& state) {
            FlatNode* flat = state.selected();
            if (flat == nullptr || flat->node->type != UiNodeType::Option || flat->node->option == nullptr) {
                return;
            }

            const ConfigOption& option = *flat->node->option;
            std::string initial;
            if (option.hasActiveValue && option.activeValue.has_value()) {
                initial = *option.activeValue;
            } else if (option.defaultValue.has_value()) {
                initial = *option.defaultValue;
            }

            const std::optional<std::string> entered = promptText(fullOptionKey(option) + " = ", initial);
            if (entered.has_value()) {
                state.setSelectedValue(*entered);
            }
        }

        // Save: if a path is already known (a prior --save-config, or a path already used to save once
        // in this session), it is offered as an editable, prefilled confirmation prompt rather than a
        // blind silent write or a blank prompt as if no path were known at all.
        std::string doSave(UiState& state, const UiOptions& uiOptions, std::optional<std::string>& savedConfigPathForRun) {
            if (!confirmRequiredOptionsOk(state.model())) {
                return "Save canceled.";
            }

            const std::optional<std::string> path = promptText("Save config to: ", defaultSavePath(uiOptions, savedConfigPathForRun));
            if (!path.has_value() || path->empty()) {
                return "Save canceled.";
            }

            const SaveOutcome outcome =
                performSaveConfig(state.model(), uiOptions.targetPath, uiOptions.targetArgTokens, *path, uiOptions.dryRun, uiOptions.keepTemp);
            showMessageBox(outcome.message.empty() ? "Done.\n" : outcome.message);
            if (outcome.succeeded) {
                savedConfigPathForRun = *path;
                state.acknowledgeSaved();
                return "Saved to " + *path + ".";
            }
            return "Save failed.";
        }

        // Suspends Curses, hands the real terminal over to the target so it runs attached and
        // foreground-visible (its own stdout/stderr/log output goes straight to the terminal, exactly as
        // if it had been started directly from a shell), waits for exactly one keypress once it exits,
        // then resumes Curses. snodec-control itself stays alive throughout: the target is never exec()'d
        // in its place.
        void waitForAnyKeyInRawTerminalMode() {
            struct termios oldTermios {};
            if (::tcgetattr(STDIN_FILENO, &oldTermios) != 0) {
                // No controlling terminal to reconfigure (unusual); fall back to a plain blocking read.
                char discard = 0;
                const ssize_t ignoredReadResult = ::read(STDIN_FILENO, &discard, 1);
                (void) ignoredReadResult;
                return;
            }

            struct termios rawTermios = oldTermios;
            rawTermios.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO));
            rawTermios.c_cc[VMIN] = 1;
            rawTermios.c_cc[VTIME] = 0;
            ::tcsetattr(STDIN_FILENO, TCSANOW, &rawTermios);

            char discard = 0;
            const ssize_t ignoredReadResult = ::read(STDIN_FILENO, &discard, 1);
            (void) ignoredReadResult;

            ::tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
        }

        std::string doRun(UiState& state, const UiOptions& uiOptions, std::optional<std::string>& savedConfigPathForRun) {
            if (!confirmRequiredOptionsOk(state.model())) {
                return "Run canceled.";
            }

            std::optional<std::string> configPath;
            std::string tempPathCreated;

            if (savedConfigPathForRun.has_value()) {
                configPath = savedConfigPathForRun;
            } else {
                erase();
                int maxX = 0;
                int maxY = 0;
                getmaxyx(stdscr, maxY, maxX);
                static_cast<void>(maxY);
                mvprintw(0, 0, "%s", fitToWidth("No saved config file yet to run with.", maxX).c_str());
                mvprintw(
                    1, 0, "%s",
                    fitToWidth("(S) Save now and run   (T) Run with a temporary materialized config   (C) Cancel", maxX).c_str());
                refresh();

                int choice = 0;
                for (;;) {
                    choice = getch();
                    if (choice == 's' || choice == 'S' || choice == 't' || choice == 'T' || choice == 'c' || choice == 'C' ||
                        choice == 27) {
                        break;
                    }
                }

                if (choice == 'c' || choice == 'C' || choice == 27) {
                    return "Run canceled.";
                }

                if (choice == 's' || choice == 'S') {
                    const std::optional<std::string> path =
                        promptText("Save config to: ", defaultSavePath(uiOptions, savedConfigPathForRun));
                    if (!path.has_value() || path->empty()) {
                        return "Run canceled.";
                    }
                    const SaveOutcome outcome = performSaveConfig(
                        state.model(), uiOptions.targetPath, uiOptions.targetArgTokens, *path, uiOptions.dryRun, uiOptions.keepTemp);
                    showMessageBox(outcome.message.empty() ? "Done.\n" : outcome.message);
                    if (!outcome.succeeded) {
                        return "Run canceled: save failed.";
                    }
                    savedConfigPathForRun = *path;
                    state.acknowledgeSaved();
                    configPath = *path;
                } else {
                    if (uiOptions.dryRun) {
                        configPath = "<generated-temp-config>";
                    } else {
                        std::string error;
                        const std::string tempPath = createTempFile(materialize(state.model(), uiOptions.targetPath), error);
                        if (tempPath.empty()) {
                            showMessageBox("Error: could not create temporary materialized config: " + error + "\n");
                            return "Run canceled.";
                        }
                        configPath = tempPath;
                        tempPathCreated = tempPath;
                    }
                }
            }

            const auto runArgsOpt = buildRunArgs(uiOptions.targetArgTokens, configPath);
            std::string status;

            if (!runArgsOpt) {
                showMessageBox("Error: --target-args already specifies a config file; refusing to also append run arguments "
                                "(conflict).\n");
                status = "Run canceled: config-file conflict.";
            } else if (uiOptions.dryRun) {
                showMessageBox("[dry-run] Would run: " + formatCommandForDisplay(uiOptions.targetPath, *runArgsOpt) + "\n");
                status = "Dry-run: target was not actually executed.";
            } else {
                // Suspend Curses and hand the real terminal to the target. def_prog_mode()/endwin() is
                // the standard Curses idiom for temporarily shelling out; reset_prog_mode() + refresh()
                // afterwards restores Curses' own terminal state without a full re-initscr().
                def_prog_mode();
                endwin();

                std::cout << "\n--- Handing the terminal to: " << formatCommandForDisplay(uiOptions.targetPath, *runArgsOpt)
                          << " ---\n";
                std::cout.flush();

                // Unlike the plain CLI's --run (runProcessAttached()), the target here is placed in its
                // own process group and made the terminal's foreground job, so Ctrl-C/Ctrl-\/Ctrl-Z stop
                // the target only: snodec-control must survive to restore the Curses UI afterward.
                const ProcessResult runResult = runProcessAttachedAsForegroundJob(uiOptions.targetPath, *runArgsOpt);

                std::cout << "\nTarget exited";
                if (!runResult.spawned) {
                    std::cout << " (failed to execute: " << runResult.spawnError << ")";
                } else if (runResult.signaled) {
                    std::cout << " (terminated by signal " << runResult.signalNumber << ")";
                } else {
                    std::cout << " with status " << runResult.exitCode;
                }
                std::cout << ". Press any key to return to snodec-control..." << std::flush;

                waitForAnyKeyInRawTerminalMode();

                reset_prog_mode();
                refresh();
                keypad(stdscr, TRUE);
                cbreak();
                noecho();
                curs_set(0);
                // The main loop's next render() call fully erases and redraws the tree/detail/status
                // areas from scratch, so no separate explicit redraw is needed here.

                status = "Target exited; returned to snodec-control.";
            }

            if (!tempPathCreated.empty()) {
                if (uiOptions.keepTemp) {
                    showMessageBox("Kept temporary run config at " + tempPathCreated + "\n");
                } else {
                    std::filesystem::remove(tempPathCreated);
                }
            }

            return status;
        }

        void doDiff(UiState& state, const UiOptions& uiOptions) {
            const std::vector<ChangeRecord> combined = mergeChangeRecords(uiOptions.priorChanges, state.changes());
            showMessageBox(formatDiff(combined));
        }

        void doCheckRequired(UiState& state) {
            const std::vector<const ConfigOption*> missing = missingRequiredOptions(state.model());
            showMessageBox(formatMissingRequired(missing));
        }

        std::string doMaterialize(UiState& state, const std::string& targetPath) {
            const std::optional<std::string> path = promptText("Materialize to file: ", "");
            if (!path.has_value() || path->empty()) {
                return "Materialize canceled.";
            }

            const std::string materialized = materialize(state.model(), targetPath);
            std::string error;
            if (!writeFile(*path, materialized, error)) {
                showMessageBox("Error: " + error + "\n");
                return "Materialize failed.";
            }
            showMessageBox("Wrote materialized configuration to " + *path + "\n");
            return "Materialized to " + *path + ".";
        }

        void showHelp() {
            showMessageBox(
                "snodec-control interactive UI - key bindings\n"
                "\n"
                "  Up/Down, PageUp/PageDown, Home/End   Move selection\n"
                "  Left                                 Collapse the selected container, or move to its parent\n"
                "  Right                                Expand the selected container, or move to its first child\n"
                "  Enter                                On a container: expand/collapse. On an option: edit its value\n"
                "  Space                                Cycle a true/false/default option only (does nothing else)\n"
                "  S                                    Save configuration through the target\n"
                "  R                                    Run the target, handing it the terminal until it exits\n"
                "  D                                    Show the diff of all changes so far\n"
                "  C                                    Check required options\n"
                "  M                                    Materialize the edited config to a file\n"
                "  H, F1                                This help screen\n"
                "  Q                                    Quit (prompts to keep/discard if there are unsaved changes)\n");
        }

        void render(UiState& state, const UiOptions& uiOptions, const std::string& statusMessage, std::size_t& scrollOffset) {
            erase();
            int maxY = 0;
            int maxX = 0;
            getmaxyx(stdscr, maxY, maxX);

            const int detailHeight = 6;
            const int treeHeight = maxY - 1 - detailHeight - 1;

            attron(A_REVERSE);
            std::string title = "snodec-control";
            if (state.isDirty()) {
                title += " [Modified]";
            }
            title += "  --  " + uiOptions.targetPath;
            // The dirty marker must stay visible even when targetPath alone would overflow the width, so
            // truncate the tail (the path), never the front, via fitToWidth().
            mvprintw(0, 0, "%s", fitToWidth(title, maxX).c_str());
            attroff(A_REVERSE);

            const std::vector<FlatNode>& rows = state.visibleRows();
            const std::size_t selected = state.selectedIndex();

            if (treeHeight > 0) {
                if (selected < scrollOffset) {
                    scrollOffset = selected;
                }
                if (selected >= scrollOffset + static_cast<std::size_t>(treeHeight)) {
                    scrollOffset = selected - static_cast<std::size_t>(treeHeight) + 1;
                }
            }

            for (int row = 0; row < treeHeight; ++row) {
                const std::size_t index = scrollOffset + static_cast<std::size_t>(row);
                if (index >= rows.size()) {
                    break;
                }

                const UiNode& node = *rows[index].node;
                std::string text(static_cast<std::size_t>(rows[index].depth) * 2, ' ');
                text += nodeLineLabel(node);

                const bool isSelected = index == selected;
                const bool missingRequired = node.type == UiNodeType::Option && node.option != nullptr && isOptionMissingRequired(*node.option);

                if (isSelected) {
                    attron(A_REVERSE);
                }
                if (missingRequired) {
                    attron(A_BOLD);
                }
                mvprintw(1 + row, 0, "%s", fitToWidth(text, maxX).c_str());
                if (missingRequired) {
                    attroff(A_BOLD);
                }
                if (isSelected) {
                    attroff(A_REVERSE);
                }
            }

            const int detailTop = 1 + treeHeight;
            mvhline(detailTop, 0, ACS_HLINE, maxX);

            if (selected < rows.size()) {
                const UiNode& node = *rows[selected].node;
                const std::string detail = node.type == UiNodeType::Option && node.option != nullptr ? formatOptionBlock(*node.option)
                                                                                                       : describeContainer(node);
                printLines(detail, detailTop + 1, maxY, maxX);
            }

            attron(A_REVERSE);
            const std::string status = statusMessage.empty()
                                            ? "Up/Down Move  Left Parent  Right Child  Enter Edit  Space Bool  S Save  R Run  D Diff  "
                                              "C Check  M Materialize  H Help  Q Quit"
                                            : statusMessage;
            mvprintw(maxY - 1, 0, "%s", fitToWidth(status, maxX).c_str());
            attroff(A_REVERSE);

            refresh();
        }

    } // namespace

    bool isUiAvailable() {
        return true;
    }

    UiResult runInteractiveUi(ConfigModel& model, const UiOptions& uiOptions) {
        CursesSession session;
        UiState state(model, uiOptions.metadata);
        std::optional<std::string> savedConfigPathForRun;
        std::string statusMessage;
        std::size_t scrollOffset = 0;

        bool quit = false;
        while (!quit) {
            render(state, uiOptions, statusMessage, scrollOffset);
            statusMessage.clear();

            const int ch = getch();
            switch (ch) {
                case KEY_UP:
                    state.moveSelection(-1);
                    break;
                case KEY_DOWN:
                    state.moveSelection(1);
                    break;
                case KEY_PPAGE:
                    state.moveSelection(-10);
                    break;
                case KEY_NPAGE:
                    state.moveSelection(10);
                    break;
                case KEY_HOME:
                    state.moveToFirst();
                    break;
                case KEY_END:
                    state.moveToLast();
                    break;
                case KEY_LEFT: {
                    FlatNode* flat = state.selected();
                    if (flat == nullptr) {
                        break;
                    }
                    if (flat->node->type != UiNodeType::Option && flat->node->expanded) {
                        state.toggleExpandSelected();
                    } else {
                        state.moveToParent();
                    }
                    break;
                }
                case KEY_RIGHT: {
                    FlatNode* flat = state.selected();
                    if (flat == nullptr) {
                        break;
                    }
                    if (flat->node->type == UiNodeType::Option) {
                        statusMessage = "Option nodes cannot be expanded. Press Enter to edit.";
                    } else if (!flat->node->expanded) {
                        state.toggleExpandSelected();
                    } else {
                        state.moveToFirstChild();
                    }
                    break;
                }
                case '\n':
                case '\r':
                case KEY_ENTER: {
                    FlatNode* flat = state.selected();
                    if (flat == nullptr) {
                        break;
                    }
                    if (flat->node->type == UiNodeType::Option) {
                        editSelectedOption(state);
                    } else {
                        state.toggleExpandSelected();
                    }
                    break;
                }
                case ' ': {
                    FlatNode* flat = state.selected();
                    if (flat != nullptr && flat->node->type == UiNodeType::Option && flat->node->option != nullptr &&
                        (isBooleanLikeOption(*flat->node->option) || isTristateLikeOption(*flat->node->option))) {
                        state.cycleSelectedBoolean();
                    } else {
                        statusMessage = "Space toggles only true/false/default options; press Enter to edit.";
                    }
                    break;
                }
                case 's':
                case 'S':
                    statusMessage = doSave(state, uiOptions, savedConfigPathForRun);
                    break;
                case 'r':
                case 'R':
                    statusMessage = doRun(state, uiOptions, savedConfigPathForRun);
                    break;
                case 'd':
                case 'D':
                    doDiff(state, uiOptions);
                    break;
                case 'c':
                case 'C':
                    doCheckRequired(state);
                    break;
                case 'm':
                case 'M':
                    statusMessage = doMaterialize(state, uiOptions.targetPath);
                    break;
                case 'h':
                case 'H':
                case KEY_F(1):
                    showHelp();
                    break;
                case 'q':
                case 'Q': {
                    if (!state.isDirty()) {
                        quit = true;
                        break;
                    }

                    erase();
                    int maxY = 0;
                    int maxX = 0;
                    getmaxyx(stdscr, maxY, maxX);
                    printLines(formatDiff(state.changes()), 0, maxY, maxX);
                    attron(A_REVERSE);
                    mvprintw(maxY - 1, 0, "%s", fitToWidth("(K) Keep changes and quit   (D) Discard changes and quit   (C) Cancel", maxX).c_str());
                    attroff(A_REVERSE);
                    refresh();

                    int choice = 0;
                    for (;;) {
                        choice = getch();
                        if (choice == 'k' || choice == 'K' || choice == 'd' || choice == 'D' || choice == 'c' || choice == 'C' ||
                            choice == 27) {
                            break;
                        }
                    }

                    if (choice == 'k' || choice == 'K') {
                        quit = true;
                    } else if (choice == 'd' || choice == 'D') {
                        state.discard();
                        quit = true;
                    }
                    // 'c'/'C'/Escape: cancel, stay in the UI.
                    break;
                }
                default:
                    break;
            }
        }

        UiResult result;
        result.changes = state.changes();
        result.savedConfigPathForRun = savedConfigPathForRun;
        return result;
    }

} // namespace snodec::control::ui
