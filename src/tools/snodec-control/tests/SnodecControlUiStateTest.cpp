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

#include "ConfigEditor.h"
#include "ConfigModel.h"
#include "ConfigParser.h"
#include "TestResult.h"
#include "ui/UiState.h"
#include "ui/UiTree.h"

#include <string>

using snodec::control::ConfigModel;
using snodec::control::ConfigOption;
using snodec::control::isOptionMissingRequired;
using snodec::control::parseShowConfigOutput;
using snodec::control::ui::FlatNode;
using snodec::control::ui::isBooleanLikeOption;
using snodec::control::ui::isTristateLikeOption;
using snodec::control::ui::nextCycledValue;
using snodec::control::ui::UiNodeType;
using snodec::control::ui::UiState;

namespace {

    ConfigModel sampleModel() {
        const std::string input =
            "# Log level\n"
            "#log-level=4\n"
            "log-level=4\n"
            "\n"
            // Boolean, default false, currently unset: effective value is "false" purely via fallback.
            // This is exactly the scenario the bug report described ("default is false, no active
            // value... pressing Space visually changes nothing"): cycling must still flip it to "true".
            "# Bool, default false, unset\n"
            "#feature.boolDefaultFalseUnset=false\n"
            "\n"
            "# Bool, default true, unset\n"
            "#feature.boolDefaultTrueUnset=true\n"
            "\n"
            "# Bool, explicit active false\n"
            "#feature.boolActiveFalse=false\n"
            "feature.boolActiveFalse=false\n"
            "\n"
            "# Bool, explicit active true, no default at all\n"
            "feature.boolActiveTrueNoDefault=true\n"
            "\n"
            // Tristate: default is literally the string "default" (SNode.C's own tristate marker).
            "# Tristate, explicit active false\n"
            "#feature.tristateActiveFalse=default\n"
            "feature.tristateActiveFalse=false\n"
            "\n"
            "# Tristate, explicit active literally \"default\"\n"
            "#feature.tristateActiveDefault=default\n"
            "feature.tristateActiveDefault=default\n"
            "\n"
            "# Tristate, currently unset (effective value is \"default\" via fallback)\n"
            "#feature.tristateUnset=default\n"
            "\n"
            "# Non-boolean string option\n"
            "#echoserver.local.host=0.0.0.0\n"
            "echoserver.local.host=0.0.0.0\n"
            "\n"
            "# Local port (required)\n"
            "#echoserver.local.port=\"<REQUIRED>\"\n";

        return parseShowConfigOutput(input).model;
    }

    // Moves the selection to the first visible row whose Option label matches `label`, expanding
    // ancestor containers as needed. Returns false if no such row is found after expanding everything.
    bool selectOptionByLabel(UiState& state, const std::string& label) {
        // Expand every container repeatedly until the tree is fully unfolded, then search.
        for (int pass = 0; pass < 8; ++pass) {
            const auto& rows = state.visibleRows();
            for (std::size_t i = 0; i < rows.size(); ++i) {
                if (rows[i].node->type != UiNodeType::Option && !rows[i].node->expanded) {
                    // Force-expand without needing to be selected first, for test setup convenience.
                    rows[i].node->expanded = true;
                }
            }
            state.rebuildTree();
        }

        const auto& rows = state.visibleRows();
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].node->type == UiNodeType::Option && rows[i].node->label == label) {
                while (state.selectedIndex() != i) {
                    state.moveSelection(state.selectedIndex() < i ? 1 : -1);
                }
                return true;
            }
        }
        return false;
    }

    void testSelectionMovement(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectEqual(0, static_cast<int>(state.selectedIndex()), "selection: starts at row 0");

        state.moveSelection(1);
        testResult.expectEqual(1, static_cast<int>(state.selectedIndex()), "selection: moves down by one");

        state.moveSelection(-100);
        testResult.expectEqual(0, static_cast<int>(state.selectedIndex()), "selection: clamps at the top");

        state.moveToLast();
        const std::size_t last = state.selectedIndex();
        state.moveSelection(100);
        testResult.expectEqual(static_cast<int>(last), static_cast<int>(state.selectedIndex()), "selection: clamps at the bottom");

        state.moveToFirst();
        testResult.expectEqual(0, static_cast<int>(state.selectedIndex()), "selection: moveToFirst returns to row 0");
    }

    void testSetAndDirtyTracking(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(!state.isDirty(), "dirty: a freshly constructed UiState is not dirty");

        testResult.expectTrue(selectOptionByLabel(state, "log-level"), "dirty: 'log-level' option is reachable in the tree");
        testResult.expectTrue(state.setSelectedValue("5"), "dirty: setSelectedValue succeeds on an Option row");
        testResult.expectTrue(state.isDirty(), "dirty: becomes true after a real edit");
        testResult.expectEqual(1, static_cast<int>(state.changes().size()), "dirty: exactly one change recorded");
        if (!state.changes().empty()) {
            testResult.expectEqual(std::string("4"), state.changes()[0].before, "dirty: 'before' reflects the original value");
            testResult.expectEqual(std::string("5"), state.changes()[0].after, "dirty: 'after' reflects the new value");
        }

        // Setting it right back to the original value nets out to "not dirty" again, matching
        // applyEdits()/mergeChangeRecords()'s no-op-collapsing behavior used everywhere else.
        testResult.expectTrue(state.setSelectedValue("4"), "dirty: setting the value back to the original succeeds");
        testResult.expectTrue(!state.isDirty(), "dirty: reverting an edit clears the dirty flag again");
    }

    void testUnsetSelected(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "log-level"), "unset: 'log-level' is reachable");
        testResult.expectTrue(state.setSelectedValue("9"), "unset: initial edit succeeds");
        testResult.expectTrue(state.unsetSelectedValue(), "unset: unsetSelectedValue succeeds on the same row");

        // Matches applyEdits()'s existing --unset semantics (see SnodecControlConfigEditorTest.cpp): an
        // unset after a prior active value is still a recorded change ("9" -> the literal "<unset>"),
        // even though the option's effective displayed value falls back to the same default ("4") it
        // started at. So the session is still dirty; it did not silently revert to a no-op.
        testResult.expectTrue(state.isDirty(), "unset: unset-after-set is still a recorded, dirty change (not silently a no-op)");
        testResult.expectEqual(1, static_cast<int>(state.changes().size()), "unset: exactly one merged change remains");
        if (!state.changes().empty()) {
            testResult.expectEqual(std::string("4"), state.changes()[0].before, "unset: merged 'before' is the original value");
            testResult.expectEqual(std::string("<unset>"), state.changes()[0].after, "unset: merged 'after' is the literal <unset>");
        }

        const FlatNode* selected = state.selected();
        testResult.expectTrue(selected != nullptr && selected->node->option != nullptr, "unset: selection still resolves");
        if (selected != nullptr && selected->node->option != nullptr) {
            testResult.expectTrue(!selected->node->option->hasActiveValue, "unset: the option itself has no active value anymore");
            testResult.expectEqual(
                std::string("4"), *selected->node->option->defaultValue, "unset: falls back to the original default for display purposes");
        }
    }

    // The exact bug reported: default false, no active value (effective value is "false" only via
    // fallback). Space must still visibly flip it to "true", not silently "set explicit false".
    void testBooleanDefaultFalseUnsetCyclesToTrue(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "boolDefaultFalseUnset"), "bool default-false-unset: option is reachable");
        const ConfigOption* option = state.selected()->node->option;

        testResult.expectTrue(!option->hasActiveValue, "bool default-false-unset: fixture starts with no active value");
        testResult.expectTrue(isBooleanLikeOption(*option), "bool default-false-unset: recognized as boolean-like");
        testResult.expectTrue(!isTristateLikeOption(*option), "bool default-false-unset: not tristate");

        testResult.expectTrue(state.cycleSelectedBoolean(), "bool default-false-unset: Space succeeds");
        testResult.expectTrue(
            option->hasActiveValue && *option->activeValue == "true", "bool default-false-unset: Space -> explicit active true");
    }

    void testBooleanDefaultTrueUnsetCyclesToFalse(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "boolDefaultTrueUnset"), "bool default-true-unset: option is reachable");
        const ConfigOption* option = state.selected()->node->option;

        testResult.expectTrue(!option->hasActiveValue, "bool default-true-unset: fixture starts with no active value");
        testResult.expectTrue(isBooleanLikeOption(*option), "bool default-true-unset: recognized as boolean-like");

        testResult.expectTrue(state.cycleSelectedBoolean(), "bool default-true-unset: Space succeeds");
        testResult.expectTrue(
            option->hasActiveValue && *option->activeValue == "false", "bool default-true-unset: Space -> explicit active false");
    }

    void testBooleanActiveFalseCyclesToTrue(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "boolActiveFalse"), "bool active-false: option is reachable");
        const ConfigOption* option = state.selected()->node->option;

        testResult.expectTrue(state.cycleSelectedBoolean(), "bool active-false: Space succeeds");
        testResult.expectTrue(option->hasActiveValue && *option->activeValue == "true", "bool active-false: Space -> true");
    }

    void testBooleanActiveTrueNoDefaultCyclesToFalse(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(
            selectOptionByLabel(state, "boolActiveTrueNoDefault"), "bool active-true-no-default: option is reachable");
        const ConfigOption* option = state.selected()->node->option;

        testResult.expectTrue(!option->defaultValue.has_value(), "bool active-true-no-default: fixture truly has no default");
        testResult.expectTrue(isBooleanLikeOption(*option), "bool active-true-no-default: recognized as boolean-like via active value");

        testResult.expectTrue(state.cycleSelectedBoolean(), "bool active-true-no-default: Space succeeds");
        testResult.expectTrue(option->hasActiveValue && *option->activeValue == "false", "bool active-true-no-default: Space -> false");

        testResult.expectTrue(state.cycleSelectedBoolean(), "bool active-true-no-default: second Space succeeds");
        testResult.expectTrue(
            option->hasActiveValue && *option->activeValue == "true", "bool active-true-no-default: second Space -> true again");
    }

    // The documented tristate cycle "false -> true -> default -> false", starting from an explicit
    // active "false", exercised via nextCycledValue() directly and via three successive Space presses.
    void testTristateCycleFromFalse(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "tristateActiveFalse"), "tristate from false: option is reachable");
        const ConfigOption* option = state.selected()->node->option;

        testResult.expectTrue(isTristateLikeOption(*option), "tristate from false: recognized as tristate (default is literally 'default')");
        testResult.expectTrue(!isBooleanLikeOption(*option), "tristate from false: tristate takes precedence over boolean-like");

        testResult.expectTrue(state.cycleSelectedBoolean(), "tristate from false: 1st cycle succeeds");
        testResult.expectTrue(option->hasActiveValue && *option->activeValue == "true", "tristate: false -> true");

        testResult.expectTrue(state.cycleSelectedBoolean(), "tristate from false: 2nd cycle succeeds");
        testResult.expectTrue(
            option->hasActiveValue && *option->activeValue == "default", "tristate: true -> default (explicit, not merely unset)");

        testResult.expectTrue(state.cycleSelectedBoolean(), "tristate from false: 3rd cycle succeeds");
        testResult.expectTrue(option->hasActiveValue && *option->activeValue == "false", "tristate: default -> false (wraps around)");
    }

    // Same cycle, but starting from an option whose active value is already explicitly the literal
    // string "default" (as opposed to being merely unset and falling back to it).
    void testTristateCycleFromExplicitDefault(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "tristateActiveDefault"), "tristate from explicit default: option is reachable");
        const ConfigOption* option = state.selected()->node->option;
        testResult.expectTrue(
            option->hasActiveValue && *option->activeValue == "default", "tristate from explicit default: fixture starts at 'default'");

        testResult.expectTrue(state.cycleSelectedBoolean(), "tristate from explicit default: cycle succeeds");
        testResult.expectTrue(option->hasActiveValue && *option->activeValue == "false", "tristate: default -> false");
    }

    // A tristate option that has never had an active value set at all (effective value "default" via
    // fallback) must cycle identically to one whose active value was explicitly "default".
    void testTristateCycleFromUnset(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "tristateUnset"), "tristate unset: option is reachable");
        const ConfigOption* option = state.selected()->node->option;
        testResult.expectTrue(!option->hasActiveValue, "tristate unset: fixture starts with no active value");

        testResult.expectTrue(state.cycleSelectedBoolean(), "tristate unset: cycle succeeds");
        testResult.expectTrue(option->hasActiveValue && *option->activeValue == "false", "tristate unset: (fallback) default -> false");
    }

    void testSpaceOnNonBooleanOptionIsRejected(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "host"), "non-boolean: 'host' (0.0.0.0) is reachable");
        const ConfigOption* option = state.selected()->node->option;

        testResult.expectTrue(!isBooleanLikeOption(*option), "non-boolean: not recognized as boolean-like");
        testResult.expectTrue(!isTristateLikeOption(*option), "non-boolean: not recognized as tristate-like");
        testResult.expectTrue(!nextCycledValue(*option).has_value(), "non-boolean: nextCycledValue() is nullopt");
        testResult.expectTrue(!state.cycleSelectedBoolean(), "non-boolean: cycleSelectedBoolean() returns false, no edit");
        testResult.expectTrue(!state.isDirty(), "non-boolean: rejected cycle attempt does not mark the state dirty");
    }

    void testRequiredMarkerHelper(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "port"), "required marker: 'port' is reachable");
        const FlatNode* selected = state.selected();
        testResult.expectTrue(selected != nullptr && selected->node->option != nullptr, "required marker: selection resolves");
        if (selected == nullptr || selected->node->option == nullptr) {
            return;
        }

        testResult.expectTrue(isOptionMissingRequired(*selected->node->option), "required marker: unset required option is flagged");

        state.setSelectedValue("8080");
        testResult.expectTrue(!isOptionMissingRequired(*selected->node->option), "required marker: clears once a value is set");
        testResult.expectTrue(state.isDirty(), "required marker: setting the value also marks the session dirty");
        testResult.expectEqual(1, static_cast<int>(state.changes().size()), "required marker: the edit is reflected in changes()");
    }

    void testDiscardRestoresPristineState(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "log-level"), "discard: 'log-level' is reachable");
        testResult.expectTrue(state.setSelectedValue("42"), "discard: edit succeeds");
        testResult.expectTrue(state.isDirty(), "discard: state is dirty before discard");

        state.discard();

        testResult.expectTrue(!state.isDirty(), "discard: no longer dirty after discard");
        testResult.expectTrue(state.changes().empty(), "discard: accumulated changes are cleared");

        // The tree was rebuilt against the restored model; re-locating the option and reading its value
        // through the (now different) cached pointer must reflect the original value, not "42".
        testResult.expectTrue(selectOptionByLabel(state, "log-level"), "discard: 'log-level' is still reachable after rebuild");
        const FlatNode* selected = state.selected();
        testResult.expectTrue(selected != nullptr && selected->node->option != nullptr, "discard: selection resolves after rebuild");
        if (selected != nullptr && selected->node->option != nullptr) {
            testResult.expectEqual(std::string("4"), *selected->node->option->activeValue, "discard: value is restored to the original");
        }
    }

    // Requirement 1: after a successful in-UI Save (modeled here as the CLI would call it,
    // acknowledgeSaved()), the session must no longer be dirty, even though the underlying edit is still
    // present in changes() for the eventual post-UI --diff.
    void testAcknowledgeSavedClearsDirtyButKeepsChanges(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        testResult.expectTrue(selectOptionByLabel(state, "log-level"), "save-ack: 'log-level' is reachable");
        testResult.expectTrue(state.setSelectedValue("7"), "save-ack: edit succeeds");
        testResult.expectTrue(state.isDirty(), "save-ack: dirty before acknowledging a save");

        state.acknowledgeSaved();

        testResult.expectTrue(!state.isDirty(), "save-ack: acknowledgeSaved() clears the dirty flag (Modified disappears, Q would not prompt)");
        testResult.expectEqual(
            1, static_cast<int>(state.changes().size()), "save-ack: changes() still reflects the edit for the eventual --diff");

        // Editing further and then reverting to the just-acknowledged (saved) value should clear dirty
        // again, exactly like reverting to the original value does before any save.
        testResult.expectTrue(state.setSelectedValue("8"), "save-ack: a further edit after acknowledging succeeds");
        testResult.expectTrue(state.isDirty(), "save-ack: dirty again after a new edit past the acknowledged point");
        testResult.expectTrue(state.setSelectedValue("7"), "save-ack: reverting to the acknowledged value succeeds");
        testResult.expectTrue(!state.isDirty(), "save-ack: reverting to the last-acknowledged value clears dirty again");

        // discard() always reverts all the way back to the state the UI opened with (session start),
        // regardless of any saves acknowledged in between.
        state.discard();
        testResult.expectTrue(selectOptionByLabel(state, "log-level"), "save-ack: 'log-level' reachable after discard");
        testResult.expectEqual(
            std::string("4"), *state.selected()->node->option->activeValue, "save-ack: discard reverts past the acknowledged save, to session start");
    }

    void testNonOptionRowsRejectValueEdits(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        // Row 0 is always a container (Application Options), never an Option.
        state.moveToFirst();
        testResult.expectTrue(!state.setSelectedValue("x"), "non-option row: setSelectedValue fails on a container node");
        testResult.expectTrue(!state.unsetSelectedValue(), "non-option row: unsetSelectedValue fails on a container node");
        testResult.expectTrue(!state.cycleSelectedBoolean(), "non-option row: cycleSelectedBoolean fails on a container node");
        testResult.expectTrue(!state.isDirty(), "non-option row: none of the rejected calls mark the state dirty");
    }

    // Covers the Left/Right tree-navigation primitives (moveToParent/moveToFirstChild/
    // toggleExpandSelected) that CursesUi.cpp's KEY_LEFT/KEY_RIGHT handlers compose to implement
    // "Right: expand collapsed, or move to first child if already expanded" / "Left: collapse if
    // expanded, else move to parent". The key dispatch itself lives in CursesUi.cpp and needs a real
    // terminal to exercise directly; these primitives are what make that dispatch correct.
    void testParentAndChildNavigation(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        UiState state(model);

        // Row 0 (Application Options) is top-level: no parent to move to.
        state.moveToFirst();
        testResult.expectTrue(!state.moveToParent(), "nav: a top-level row has no parent to move to");

        // Application Options starts expanded with its one child (log-level): moveToFirstChild() moves
        // onto that child directly, without needing a prior expand step.
        testResult.expectTrue(state.moveToFirstChild(), "nav: moveToFirstChild() succeeds on an expanded container with children");
        testResult.expectTrue(
            state.selected() != nullptr && state.selected()->node->type == UiNodeType::Option &&
                state.selected()->node->label == "log-level",
            "nav: moveToFirstChild() from Application Options lands on log-level");

        // Left on a leaf moves to its parent section/container (here: Application Options).
        testResult.expectTrue(state.moveToParent(), "nav: moveToParent() from a leaf option succeeds");
        testResult.expectTrue(
            state.selected() != nullptr && state.selected()->node->type == UiNodeType::ApplicationOptions,
            "nav: moveToParent() from log-level lands back on Application Options");

        // Move to the "echoserver" instance (an Instance node, collapsed by default: it has children in
        // the tree, but none are visible/reachable via moveToFirstChild() until it is expanded). Use a
        // fresh UiState so the tree starts out fully collapsed (default expansion only).
        ConfigModel freshModel = sampleModel();
        UiState freshState(freshModel);

        bool foundInstance = false;
        for (std::size_t i = 0; i < freshState.visibleRows().size(); ++i) {
            if (freshState.visibleRows()[i].node->type == UiNodeType::Instance && freshState.visibleRows()[i].node->label == "echoserver") {
                while (freshState.selectedIndex() != i) {
                    freshState.moveSelection(freshState.selectedIndex() < i ? 1 : -1);
                }
                foundInstance = true;
                break;
            }
        }
        testResult.expectTrue(foundInstance, "nav: the collapsed 'echoserver' Instance row is reachable");
        testResult.expectTrue(!freshState.selected()->node->expanded, "nav: 'echoserver' starts collapsed");
        testResult.expectTrue(
            !freshState.moveToFirstChild(), "nav: moveToFirstChild() fails on a collapsed container (Right must expand it first)");

        // Right expands a collapsed instance (this is exactly what CursesUi.cpp's KEY_RIGHT does first).
        const std::size_t instanceIndex = freshState.selectedIndex();
        freshState.toggleExpandSelected();
        testResult.expectTrue(freshState.selected()->node->expanded, "nav: toggleExpandSelected() expands 'echoserver'");
        testResult.expectEqual(
            static_cast<int>(instanceIndex), static_cast<int>(freshState.selectedIndex()),
            "nav: selection stays on the same node across an expand (selection remains valid)");

        // Right on the now-expanded instance moves to its first child (the "Sections" root).
        testResult.expectTrue(freshState.moveToFirstChild(), "nav: moveToFirstChild() succeeds once 'echoserver' is expanded");
        testResult.expectTrue(
            freshState.selected()->node->type == UiNodeType::SectionsRoot, "nav: first child of 'echoserver' is its Sections root");

        // The Sections root is expanded by default; its first child is the "local" Section, itself
        // collapsed by default.
        testResult.expectTrue(freshState.moveToFirstChild(), "nav: moveToFirstChild() into the Sections root succeeds");
        testResult.expectTrue(freshState.selected()->node->type == UiNodeType::Section, "nav: lands on the 'local' Section");
        testResult.expectTrue(!freshState.selected()->node->expanded, "nav: 'local' Section starts collapsed");

        // Left on a collapsed section (nothing to collapse) moves to its parent instead.
        const std::size_t sectionIndex = freshState.selectedIndex();
        testResult.expectTrue(freshState.moveToParent(), "nav: Left on a collapsed section moves to its parent (Sections root)");
        testResult.expectTrue(freshState.selected()->node->type == UiNodeType::SectionsRoot, "nav: parent of 'local' is the Sections root");

        // Move back onto "local" and expand it, then verify Left collapses it in place (selection stays
        // on "local" itself, only its children disappear) rather than moving to its parent.
        while (freshState.selectedIndex() != sectionIndex) {
            freshState.moveSelection(freshState.selectedIndex() < sectionIndex ? 1 : -1);
        }
        freshState.toggleExpandSelected(); // Right: expand "local"
        testResult.expectTrue(freshState.selected()->node->expanded, "nav: 'local' is now expanded");
        const std::size_t rowsWhileExpanded = freshState.visibleRows().size();

        freshState.toggleExpandSelected(); // Left: collapse "local" again (selection unchanged)
        testResult.expectTrue(!freshState.selected()->node->expanded, "nav: 'local' collapses back");
        testResult.expectEqual(
            static_cast<int>(sectionIndex), static_cast<int>(freshState.selectedIndex()),
            "nav: selection remains on 'local' itself after collapsing it (selection remains valid)");
        testResult.expectTrue(
            freshState.visibleRows().size() < rowsWhileExpanded, "nav: collapsing 'local' actually hides its two child options again");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testSelectionMovement(testResult);
    testSetAndDirtyTracking(testResult);
    testUnsetSelected(testResult);
    testBooleanDefaultFalseUnsetCyclesToTrue(testResult);
    testBooleanDefaultTrueUnsetCyclesToFalse(testResult);
    testBooleanActiveFalseCyclesToTrue(testResult);
    testBooleanActiveTrueNoDefaultCyclesToFalse(testResult);
    testTristateCycleFromFalse(testResult);
    testTristateCycleFromExplicitDefault(testResult);
    testTristateCycleFromUnset(testResult);
    testSpaceOnNonBooleanOptionIsRejected(testResult);
    testRequiredMarkerHelper(testResult);
    testDiscardRestoresPristineState(testResult);
    testAcknowledgeSavedClearsDirtyButKeepsChanges(testResult);
    testNonOptionRowsRejectValueEdits(testResult);
    testParentAndChildNavigation(testResult);

    return testResult.processResult();
}
