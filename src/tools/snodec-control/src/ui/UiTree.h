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

#ifndef SNODECCONTROL_UI_UITREE_H
#define SNODECCONTROL_UI_UITREE_H

#include "../ConfigModel.h"
#include "../Metadata.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace snodec::control::ui {

    // SNode.C configuration is presented as a hierarchy, not a flat section/option list. Two tree
    // builders exist, side by side:
    //
    //  - buildUiTree(ConfigModel&): the original dotted-name heuristic (Instances -> Instance ->
    //    Sections -> Section -> Option, else "Other Options"), unchanged since Phase 1/3. Kept
    //    working exactly as before for targets built against a SNode.C version that predates the
    //    "#@ snodec.meta ..." comment-metadata format.
    //  - buildUiTree(ConfigModel&, const ParsedMetadata&): metadata-native. When `metadata.usable()`,
    //    builds a genuine N-ary tree keyed by each node's `path` (arbitrary depth, arbitrary shape -
    //    not hardcoded to Instances/Sections), using `kind`/`kindSource` from the metadata as the
    //    authoritative classification instead of re-guessing it from dotted names. Falls back to the
    //    legacy heuristic (delegating to the single-argument overload) whenever metadata is absent or
    //    its schema/version is unrecognized, so a fleet with mixed SNode.C versions is handled
    //    transparently by one code path.
    //
    // UiNodeType::{ApplicationOptions, InstancesRoot, Instance, SectionsRoot, Section} are produced
    // only by the legacy heuristic; UiNodeType::{Node, Group} are produced only by the metadata-native
    // builder. UiNodeType::Option is shared by both. Consumers that only care about "is this a leaf
    // option or a container" (most of ui/UiState.cpp and ui/CursesUi.cpp) never need to distinguish
    // further; consumers that render a human label per container (ui/CursesUi.cpp's
    // describeContainer()) switch on the specific type.
    enum class UiNodeType { ApplicationOptions, InstancesRoot, Instance, SectionsRoot, Section, Node, Group, Option };

    struct UiNode {
        UiNodeType type = UiNodeType::Option;
        std::string label;
        ConfigOption* option = nullptr; // non-null only when type == Option
        std::vector<UiNode> children;
        bool expanded = false; // only meaningful for container types; ignored for Option leaves

        // Populated only by the metadata-native builder (default-valued/empty for every node the
        // legacy heuristic builds, which has no metadata to draw these from).
        std::string kind;              // Node: e.g. "application"/"instance"/"section"/"anonymous"/"category"/"subcommand".
                                       // Group: "persistent"/"nonpersistent"/"custom" (never "default" - see buildUiTree()).
        std::string kindSource;        // Provenance as reported by the target, e.g. "cli11-group"/"heuristic-cli11-group"/
                                       // "cli11-name"/"fallback"/"heuristic-group-name". Surfaced honestly - a
                                       // "heuristic-*" source is the target's own best guess, not a certainty.
        std::vector<std::string> path; // Node: its own node path. Group: its owning node's path.
        bool disabled = false;         // Node only; reflects the metadata's "disabled" field (CLI11 App::get_disabled()).
    };

    struct UiTree {
        std::vector<UiNode> topLevel; // ApplicationOptions, InstancesRoot, then 0+ "Other Options" Section nodes
    };

    // Derives the hierarchy from `model`'s already-parsed sections/options. `model` must outlive the
    // returned UiTree: each Option leaf caches a raw ConfigOption* into it. Derivation rules:
    //
    //  1. An option with no section prefix at all (a dot-less name) is an Application Option.
    //  2. An option whose section prefix itself contains a dot (i.e. the fully qualified name has 3+
    //     dot-separated components) is Instance = the prefix's first component, Section = the remaining
    //     dotted path taken as one flat name (not nested further), Option = the leaf key.
    //  3. An option whose section prefix has exactly one component (name has exactly 2 components, e.g.
    //     "echoserver.enabled") is ambiguous on its own: if that prefix is also a confirmed instance name
    //     from rule 2 elsewhere in the model, the option becomes a direct child of that Instance (no
    //     Section level); otherwise the prefix is treated as a plain, non-instance top-level section.
    //  4. Rule 3's "otherwise" case, and anything else that cannot be confidently placed under an
    //     Instance, is collected into one "Other Options" top-level node so it is visible, never
    //     silently discarded, even though it does not fit the Instance/Section pattern.
    //  5. Instances and their sections/options preserve the order they were discovered in (i.e. the
    //     target's own '-s' output order), which is deterministic given a fixed target.
    UiTree buildUiTree(ConfigModel& model);

    // Metadata-native tree builder. When `metadata.usable()`, builds a single root UiNode (the
    // application, `kind == "application"`), with a genuine N-ary tree of child Node/Group/Option
    // nodes assembled directly from `metadata`'s node/group/option entities, keyed by each entity's
    // own `path`/`nodePath` - not by counting dots or hardcoding a 3-level shape, so arbitrarily
    // nested, arbitrarily shaped user-defined SubCommands are represented at whatever depth they
    // actually occur. Concretely:
    //
    //  - Every metadata `node` becomes a UiNode of type Node, placed under the Node whose path is its
    //    own path with the last segment removed (its parent), or as the tree's sole top-level entry
    //    if its path is empty (the root/application node itself).
    //  - Every metadata `group` whose kind is not "default" (i.e. every group with a real name, such
    //    as "Options (persistent)"/"Options (nonpersistent)"/a custom group name - matching exactly
    //    which groups the target's own '-s' output gives a human "## <group>" heading to) becomes a
    //    UiNode of type Group, nested under its owning Node. A node's implicit default group's
    //    options are attached directly to the Node instead, so the overwhelmingly common
    //    single-default-group case never grows a pointless extra level.
    //  - Every metadata `option` is matched back to the already-parsed `model`'s ConfigOption (via its
    //    `key`, which is exactly the literal INI key both share) and placed as an Option leaf under
    //    its Group (if one was materialized for its group) or otherwise directly under its owning
    //    Node. An option present in metadata but absent from `model` (or vice versa - which should
    //    not happen given both are derived from the same capture, but is handled defensively rather
    //    than assumed impossible) is collected into one top-level "Unmatched Options" Node instead of
    //    being silently dropped, mirroring the legacy builder's "Other Options" philosophy.
    //
    // Falls back to `buildUiTree(model)` (the legacy heuristic, unchanged) whenever
    // `metadata.usable()` is false, so a target built against an older SNode.C that never emits
    // metadata is still fully supported by the same call site.
    UiTree buildUiTree(ConfigModel& model, const ParsedMetadata& metadata);

    struct FlatNode {
        UiNode* node = nullptr;
        int depth = 0;

        // Index into the same flattened row vector of this node's parent, or std::nullopt for a
        // top-level node (ApplicationOptions, InstancesRoot, or an "Other Options" fallback node). Lets
        // Left/Right tree navigation move to a parent or first child by row index alone, without needing
        // to re-walk the tree or store UiNode parent pointers.
        std::optional<std::size_t> parentIndex;
    };

    // Depth-first flattening of `tree`, descending only into nodes whose `expanded` is true. Used both
    // for rendering (each FlatNode maps to one visible row) and for selection movement (an index into the
    // returned vector). A child row always immediately follows its parent's row (before any sibling), so
    // "move to first child" is simply "the next row, if its parentIndex is the current row".
    std::vector<FlatNode> flattenVisibleNodes(UiTree& tree);

} // namespace snodec::control::ui

#endif // SNODECCONTROL_UI_UITREE_H
