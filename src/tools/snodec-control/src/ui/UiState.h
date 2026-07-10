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

#ifndef SNODECCONTROL_UI_UISTATE_H
#define SNODECCONTROL_UI_UISTATE_H

#include "../ConfigEditor.h"
#include "../ConfigModel.h"
#include "UiTree.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace snodec::control::ui {

    // The option's current effective raw value (active if set, else default, else nullopt), with none of
    // effectiveValueDisplay()'s display-only quoting/placeholders ("<REQUIRED>"/"<unset>"/""). Used by the
    // boolean/tristate helpers below, which need to reason about the literal underlying string.
    std::optional<std::string> effectiveRawValue(const ConfigOption& option);

    // True if `option`'s parsed default is literally the string "default" - SNode.C's own marker for a
    // tristate option, whose only three valid values are the literal strings "false", "true", and
    // "default" (meaning "let the target decide", as opposed to an ordinary boolean's plain on/off).
    bool isTristateLikeOption(const ConfigOption& option);

    // True if `option`'s current effective value (see effectiveRawValue()) is exactly "true" or "false"
    // and it is not tristate-like (isTristateLikeOption() takes precedence). A plain on/off option.
    bool isBooleanLikeOption(const ConfigOption& option);

    // The next value in the option's cycle, or std::nullopt if it is neither boolean- nor tristate-like
    // (Space should not touch such an option at all). A plain boolean cycles strictly
    // "false" -> "true" -> "false" based on its *current effective* value, so cycling always visibly
    // changes what is displayed regardless of whether the option started out using its active or its
    // default value. A tristate option cycles "false" -> "true" -> "default" -> "false", always setting
    // the option to one of these three literal strings explicitly (never merely unsetting it), so every
    // step is visibly distinct.
    std::optional<std::string> nextCycledValue(const ConfigOption& option);

    // Holds everything the UI needs across its interactive session, independent of curses: the live
    // model being edited, a pristine snapshot for discard, the currently derived tree/flattened rows, the
    // selected row, and the accumulated diff. Kept free of any curses dependency so its logic (movement,
    // edits, dirty tracking, boolean cycling) can be unit tested without a real terminal.
    class UiState {
    public:
        // `metadata` defaults to an empty, "not present" value, which deterministically makes
        // rebuildTree() use the legacy dotted-name heuristic (see buildUiTree() in UiTree.h) - the
        // correct behavior for callers (such as existing tests) that have no metadata to offer.
        explicit UiState(ConfigModel& model, ParsedMetadata metadata = {});

        // (Re)computes the tree from the current model state and re-flattens it, preserving the given
        // node's expansion where a node with the same type+label path still exists (best-effort; a full
        // rebuild after discard() intentionally resets to default expansion instead, see discard()).
        void rebuildTree();

        const std::vector<FlatNode>& visibleRows() const;
        std::size_t selectedIndex() const;
        FlatNode* selected();

        void moveSelection(int delta);
        void moveToFirst();
        void moveToLast();

        // Toggles expansion of the selected node if it is a container type; no-op for Option leaves.
        void toggleExpandSelected();

        // Moves the selection to the selected row's parent (per FlatNode::parentIndex). Returns false
        // (selection unchanged) if the selected row is top-level and has no parent.
        bool moveToParent();

        // Moves the selection to the selected row's first child, i.e. the very next row if (and only if)
        // it is that row's child. Returns false (selection unchanged) if the selected row is collapsed,
        // an Option leaf, or an expanded container with no children.
        bool moveToFirstChild();

        // Applies a single text edit (--set semantics) to the selected Option leaf's underlying
        // ConfigOption, recording it via the same applyEdits()/mergeChangeRecords() machinery the CLI
        // uses, so the UI's diff output is indistinguishable in format from the CLI's. Returns false (no
        // state changed) if the selected row is not an Option.
        bool setSelectedValue(const std::string& newValue);

        // Clears the selected Option leaf's active value (--unset semantics). Returns false if the
        // selected row is not an Option.
        bool unsetSelectedValue();

        // Cycles the selected Option's boolean/tristate value per nextCycledValue(). Returns false (no
        // state changed) if the selected row is not an Option, or is not boolean-/tristate-like.
        bool cycleSelectedBoolean();

        bool isDirty() const;

        // All changes accumulated since this UiState was constructed (i.e. since the UI session opened),
        // merged/collapsed exactly like applyEdits()'s own no-op detection. Never cleared by
        // acknowledgeSaved() - only discard() clears it - so the final --diff after the UI closes always
        // reflects the true, full end-to-end edit, regardless of how many times Save ran in between.
        const std::vector<ChangeRecord>& changes() const;

        // Restores the model to its state when this UiState was constructed, clears all accumulated
        // changes (both changes() and the dirty flag), and rebuilds the tree. Raw ConfigOption* pointers
        // cached in the old tree are never dereferenced again after this call: rebuildTree() always
        // replaces them together with the tree itself.
        void discard();

        // Call after a successful in-UI Save: clears isDirty() (so the title bar's "[Modified]" marker
        // disappears and a subsequent Q quits immediately without prompting) without touching changes()
        // (so the eventual post-UI --diff still reflects everything edited in this session, saved or
        // not). Deliberately does not affect discard(), which always reverts all the way back to the
        // state the UI opened with, regardless of any saves that happened in between.
        void acknowledgeSaved();

        ConfigModel& model();

    private:
        ConfigModel& modelRef;
        ConfigModel pristineSnapshot;
        ParsedMetadata metadata;
        UiTree tree;
        std::vector<FlatNode> rows;
        std::size_t selection = 0;
        std::vector<ChangeRecord> sessionChanges; // since UI open; survives acknowledgeSaved()
        std::vector<ChangeRecord> unsavedChanges; // since last acknowledge point; drives isDirty()

        bool applySingleEdit(const EditOperation& operation);
    };

} // namespace snodec::control::ui

#endif // SNODECCONTROL_UI_UISTATE_H
