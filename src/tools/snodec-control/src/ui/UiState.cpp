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

#include "UiState.h"

#include <algorithm>
#include <utility>

namespace snodec::control::ui {

    std::optional<std::string> effectiveRawValue(const ConfigOption& option) {
        if (option.hasActiveValue && option.activeValue.has_value()) {
            return option.activeValue;
        }
        if (option.defaultValue.has_value()) {
            return option.defaultValue;
        }
        return std::nullopt;
    }

    bool isTristateLikeOption(const ConfigOption& option) {
        // Metadata's "type.kind" cannot help disambiguate boolean from tristate here: SNode.C's own
        // formatter derives "boolean" for *any* flag-like option (CLI11 expected_min() == 0), which
        // covers true booleans and tristates alike (see docs/config-comment-metadata.md's note that
        // type.kind/kindSource is a heuristic on the framework side) - so the literal default value
        // "default" remains the only reliable signal for this and is kept as the sole detector. What
        // metadata *does* add: a list-type option (type.items == "list") must never be treated as a
        // scalar tristate merely because one of several space-joined values happens to read "default".
        if (option.typeItems.has_value() && *option.typeItems == "list") {
            return false;
        }
        return option.defaultValue.has_value() && *option.defaultValue == "default";
    }

    bool isBooleanLikeOption(const ConfigOption& option) {
        if (isTristateLikeOption(option)) {
            return false;
        }
        if (option.typeItems.has_value() && *option.typeItems == "list") {
            return false; // same reasoning as isTristateLikeOption(): never cycle a multi-value option
        }
        const std::optional<std::string> value = effectiveRawValue(option);
        return value.has_value() && (*value == "true" || *value == "false");
    }

    std::optional<std::string> nextCycledValue(const ConfigOption& option) {
        if (isTristateLikeOption(option)) {
            const std::string current = effectiveRawValue(option).value_or("default");
            if (current == "false") {
                return std::string("true");
            }
            if (current == "true") {
                return std::string("default");
            }
            return std::string("false"); // covers "default" (the normal wraparound) and any stray value
        }

        if (isBooleanLikeOption(option)) {
            const std::string current = *effectiveRawValue(option);
            return current == "false" ? std::string("true") : std::string("false");
        }

        return std::nullopt;
    }

    namespace {

        void collectExpandedPaths(const std::vector<UiNode>& nodes, const std::string& prefix, std::vector<std::string>& out) {
            for (const UiNode& node : nodes) {
                const std::string path = prefix + "/" + node.label;
                if (node.type != UiNodeType::Option) {
                    if (node.expanded) {
                        out.push_back(path);
                    }
                    collectExpandedPaths(node.children, path, out);
                }
            }
        }

        void applyExpandedPaths(std::vector<UiNode>& nodes, const std::string& prefix, const std::vector<std::string>& expandedPaths) {
            for (UiNode& node : nodes) {
                const std::string path = prefix + "/" + node.label;
                if (node.type != UiNodeType::Option) {
                    if (std::find(expandedPaths.begin(), expandedPaths.end(), path) != expandedPaths.end()) {
                        node.expanded = true;
                    }
                    applyExpandedPaths(node.children, path, expandedPaths);
                }
            }
        }

    } // namespace

    UiState::UiState(ConfigModel& model, ParsedMetadata metadata)
        : modelRef(model)
        , pristineSnapshot(model)
        , metadata(std::move(metadata)) {
        rebuildTree();
    }

    void UiState::rebuildTree() {
        std::vector<std::string> expandedPaths;
        collectExpandedPaths(tree.topLevel, "", expandedPaths);

        tree = buildUiTree(modelRef, metadata);

        if (!expandedPaths.empty()) {
            applyExpandedPaths(tree.topLevel, "", expandedPaths);
        }

        rows = flattenVisibleNodes(tree);
        if (selection >= rows.size()) {
            selection = rows.empty() ? 0 : rows.size() - 1;
        }
    }

    const std::vector<FlatNode>& UiState::visibleRows() const {
        return rows;
    }

    std::size_t UiState::selectedIndex() const {
        return selection;
    }

    FlatNode* UiState::selected() {
        if (selection >= rows.size()) {
            return nullptr;
        }
        return &rows[selection];
    }

    void UiState::moveSelection(int delta) {
        if (rows.empty()) {
            return;
        }

        long newIndex = static_cast<long>(selection) + delta;
        if (newIndex < 0) {
            newIndex = 0;
        }
        if (newIndex >= static_cast<long>(rows.size())) {
            newIndex = static_cast<long>(rows.size()) - 1;
        }
        selection = static_cast<std::size_t>(newIndex);
    }

    void UiState::moveToFirst() {
        selection = 0;
    }

    void UiState::moveToLast() {
        if (!rows.empty()) {
            selection = rows.size() - 1;
        }
    }

    void UiState::toggleExpandSelected() {
        FlatNode* node = selected();
        if (node == nullptr || node->node->type == UiNodeType::Option) {
            return;
        }

        node->node->expanded = !node->node->expanded;
        rows = flattenVisibleNodes(tree);
        if (selection >= rows.size()) {
            selection = rows.empty() ? 0 : rows.size() - 1;
        }
    }

    bool UiState::moveToParent() {
        const FlatNode* node = selected();
        if (node == nullptr || !node->parentIndex.has_value()) {
            return false;
        }
        selection = *node->parentIndex;
        return true;
    }

    bool UiState::moveToFirstChild() {
        if (selection + 1 >= rows.size()) {
            return false;
        }
        if (!rows[selection + 1].parentIndex.has_value() || *rows[selection + 1].parentIndex != selection) {
            return false;
        }
        selection = selection + 1;
        return true;
    }

    bool UiState::applySingleEdit(const EditOperation& operation) {
        const EditOutcome outcome = applyEdits(modelRef, {operation}, false);
        if (!outcome.errors.empty()) {
            return false;
        }

        sessionChanges = mergeChangeRecords(sessionChanges, outcome.changes);
        unsavedChanges = mergeChangeRecords(unsavedChanges, outcome.changes);
        return true;
    }

    bool UiState::setSelectedValue(const std::string& newValue) {
        const FlatNode* node = selected();
        if (node == nullptr || node->node->type != UiNodeType::Option || node->node->option == nullptr) {
            return false;
        }

        return applySingleEdit(EditOperation{EditOperation::Kind::Set, fullOptionKey(*node->node->option), newValue});
    }

    bool UiState::unsetSelectedValue() {
        const FlatNode* node = selected();
        if (node == nullptr || node->node->type != UiNodeType::Option || node->node->option == nullptr) {
            return false;
        }

        return applySingleEdit(EditOperation{EditOperation::Kind::Unset, fullOptionKey(*node->node->option), ""});
    }

    bool UiState::cycleSelectedBoolean() {
        const FlatNode* node = selected();
        if (node == nullptr || node->node->type != UiNodeType::Option || node->node->option == nullptr) {
            return false;
        }

        const std::optional<std::string> next = nextCycledValue(*node->node->option);
        if (!next.has_value()) {
            return false;
        }

        return setSelectedValue(*next);
    }

    bool UiState::isDirty() const {
        return !unsavedChanges.empty();
    }

    const std::vector<ChangeRecord>& UiState::changes() const {
        return sessionChanges;
    }

    void UiState::discard() {
        modelRef = pristineSnapshot;
        sessionChanges.clear();
        unsavedChanges.clear();
        rebuildTree();
    }

    void UiState::acknowledgeSaved() {
        unsavedChanges.clear();
    }

    ConfigModel& UiState::model() {
        return modelRef;
    }

} // namespace snodec::control::ui
