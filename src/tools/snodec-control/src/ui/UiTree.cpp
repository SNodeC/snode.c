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

#include "UiTree.h"

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

namespace snodec::control::ui {

    namespace {

        UiNode makeOptionNode(ConfigOption* option) {
            UiNode node;
            node.type = UiNodeType::Option;
            node.label = option->key;
            node.option = option;
            return node;
        }

        // Accumulates everything discovered for one instance before the final UiNode tree is assembled,
        // preserving first-seen order for both the instance's direct options and its sections.
        struct PendingInstance {
            std::string name;
            std::vector<ConfigOption*> directOptions;
            std::vector<std::string> sectionOrder;
            std::unordered_map<std::string, std::vector<ConfigOption*>> sectionOptions;
        };

    } // namespace

    UiTree buildUiTree(ConfigModel& model) {
        UiTree tree;

        UiNode appOptions;
        appOptions.type = UiNodeType::ApplicationOptions;
        appOptions.label = "Application Options";
        appOptions.expanded = true;

        std::vector<ConfigOption*> otherOptions;

        // Pass 1: an instance is "confirmed" the moment any section name has a dot in it (i.e. some
        // option's fully qualified name has 3+ components), per rule 2.
        std::unordered_set<std::string> confirmedInstances;
        for (ConfigSection& section : model.sections) {
            const std::size_t dotPos = section.name.find('.');
            if (!section.name.empty() && dotPos != std::string::npos) {
                confirmedInstances.insert(section.name.substr(0, dotPos));
            }
        }

        std::vector<PendingInstance> pendingInstances;
        std::unordered_map<std::string, std::size_t> instanceIndex;

        auto instanceBucket = [&](const std::string& name) -> PendingInstance& {
            const auto it = instanceIndex.find(name);
            if (it != instanceIndex.end()) {
                return pendingInstances[it->second];
            }
            instanceIndex[name] = pendingInstances.size();
            pendingInstances.push_back(PendingInstance{name, {}, {}, {}});
            return pendingInstances.back();
        };

        // Pass 2: classify every option per rules 1-4.
        for (ConfigSection& section : model.sections) {
            for (ConfigOption& option : section.options) {
                const std::string& sectionName = section.name;

                if (sectionName.empty()) {
                    appOptions.children.push_back(makeOptionNode(&option));
                    continue;
                }

                const std::size_t dotPos = sectionName.find('.');
                if (dotPos != std::string::npos) {
                    const std::string instanceName = sectionName.substr(0, dotPos);
                    const std::string sectionPath = sectionName.substr(dotPos + 1);

                    PendingInstance& bucket = instanceBucket(instanceName);
                    if (bucket.sectionOptions.find(sectionPath) == bucket.sectionOptions.end()) {
                        bucket.sectionOrder.push_back(sectionPath);
                    }
                    bucket.sectionOptions[sectionPath].push_back(&option);
                    continue;
                }

                if (confirmedInstances.find(sectionName) != confirmedInstances.end()) {
                    instanceBucket(sectionName).directOptions.push_back(&option);
                } else {
                    otherOptions.push_back(&option);
                }
            }
        }

        tree.topLevel.push_back(std::move(appOptions));

        UiNode instancesRoot;
        instancesRoot.type = UiNodeType::InstancesRoot;
        instancesRoot.label = "Instances";
        instancesRoot.expanded = true;

        for (PendingInstance& bucket : pendingInstances) {
            UiNode instanceNode;
            instanceNode.type = UiNodeType::Instance;
            instanceNode.label = bucket.name;

            for (ConfigOption* option : bucket.directOptions) {
                instanceNode.children.push_back(makeOptionNode(option));
            }

            if (!bucket.sectionOrder.empty()) {
                UiNode sectionsRoot;
                sectionsRoot.type = UiNodeType::SectionsRoot;
                sectionsRoot.label = "Sections";
                sectionsRoot.expanded = true;

                for (const std::string& sectionPath : bucket.sectionOrder) {
                    UiNode sectionNode;
                    sectionNode.type = UiNodeType::Section;
                    sectionNode.label = sectionPath;

                    for (ConfigOption* option : bucket.sectionOptions[sectionPath]) {
                        sectionNode.children.push_back(makeOptionNode(option));
                    }

                    sectionsRoot.children.push_back(std::move(sectionNode));
                }

                instanceNode.children.push_back(std::move(sectionsRoot));
            }

            instancesRoot.children.push_back(std::move(instanceNode));
        }

        tree.topLevel.push_back(std::move(instancesRoot));

        if (!otherOptions.empty()) {
            UiNode other;
            other.type = UiNodeType::Section;
            other.label = "Other Options";

            for (ConfigOption* option : otherOptions) {
                other.children.push_back(makeOptionNode(option));
            }

            tree.topLevel.push_back(std::move(other));
        }

        return tree;
    }

    namespace {

        constexpr char pathSeparator = '\x1f';
        constexpr char groupKeySeparator = '\x1e';

        // Every option shown in the config UI is already persistent/configurable (SNode.C's formatter
        // never emits a non-configurable option into "-s" output at all - see
        // src/utils/Formatter.cpp's "if (!opt->get_configurable()) continue;"). So a group that is
        // just the implicit "this option is configurable" bucket - "default" kind, "persistent" kind,
        // the literal names "Options"/"Options (persistent)", or no name at all - carries no
        // information the UI needs to show as its own tree level; every option in it is shown, just
        // attached directly to its owning node instead of nested one level deeper under a redundant
        // "Options (persistent)" node.
        bool isImplicitOptionsGroup(const MetaGroup& group) {
            return group.name.empty() || group.kind == "default" || group.kind == "persistent" || group.name == "Options" ||
                   group.name == "Options (persistent)";
        }

        // "Nonpersistent Options"/"Options (nonpersistent)" is SNode.C's own label for
        // configurable(false) options - which, per the same Formatter.cpp guard, never actually reach
        // "-s" output or metadata in practice. Handled defensively anyway: the plain config editor is
        // about writable/persistent options, so a group that does turn up with this kind/name is
        // excluded from the tree entirely (not merely flattened, and not shown as "Unmatched Options"
        // either) rather than presented as something the user can edit.
        bool isNonpersistentOptionsGroup(const MetaGroup& group) {
            return group.kind == "nonpersistent" || group.name == "Options (nonpersistent)";
        }

        std::string joinPath(const std::vector<std::string>& path) {
            std::string key;
            for (const std::string& segment : path) {
                key += segment;
                key += pathSeparator;
            }
            return key;
        }

        // Assembles one Node UiNode (and, recursively, everything under it) directly from the
        // metadata-derived maps built by buildUiTree(ConfigModel&, const ParsedMetadata&) below. Every
        // option actually attached anywhere in the tree is recorded into `matchedOptions`, so the
        // caller can collect whatever is left over into a visible fallback bucket instead of silently
        // dropping it.
        UiNode assembleMetadataNode(const std::string& pathKey,
                                    bool isRoot,
                                    const std::unordered_map<std::string, std::size_t>& nodeIndexByPath,
                                    const std::vector<MetaNode>& nodes,
                                    const std::unordered_map<std::string, std::vector<std::string>>& childPathKeysByParent,
                                    const std::unordered_map<std::string, std::vector<const MetaGroup*>>& groupsByNodePath,
                                    const std::unordered_map<std::string, std::vector<const MetaOption*>>& defaultOptionsByNodePath,
                                    const std::unordered_map<std::string, std::vector<const MetaOption*>>& groupOptionsByKey,
                                    const std::unordered_map<std::string, ConfigOption*>& modelByKey,
                                    std::unordered_set<ConfigOption*>& matchedOptions) {
            const MetaNode& meta = nodes[nodeIndexByPath.at(pathKey)];

            UiNode node;
            node.type = UiNodeType::Node;
            node.kind = meta.kind;
            node.kindSource = meta.kindSource;
            node.label = !meta.displayName.empty() ? meta.displayName : (isRoot ? "Application" : meta.name);
            node.path = meta.path;
            node.disabled = meta.disabled;
            node.expanded = isRoot;

            if (const auto it = groupsByNodePath.find(pathKey); it != groupsByNodePath.end()) {
                for (const MetaGroup* group : it->second) {
                    UiNode groupNode;
                    groupNode.type = UiNodeType::Group;
                    groupNode.kind = group->kind;
                    groupNode.kindSource = group->kindSource;
                    groupNode.label = group->name;
                    groupNode.path = group->nodePath;

                    const std::string groupKey = pathKey + groupKeySeparator + group->name;
                    if (const auto optIt = groupOptionsByKey.find(groupKey); optIt != groupOptionsByKey.end()) {
                        for (const MetaOption* option : optIt->second) {
                            if (const auto modelIt = modelByKey.find(option->key); modelIt != modelByKey.end()) {
                                groupNode.children.push_back(makeOptionNode(modelIt->second));
                                matchedOptions.insert(modelIt->second);
                            }
                        }
                    }
                    node.children.push_back(std::move(groupNode));
                }
            }

            if (const auto it = defaultOptionsByNodePath.find(pathKey); it != defaultOptionsByNodePath.end()) {
                for (const MetaOption* option : it->second) {
                    if (const auto modelIt = modelByKey.find(option->key); modelIt != modelByKey.end()) {
                        node.children.push_back(makeOptionNode(modelIt->second));
                        matchedOptions.insert(modelIt->second);
                    }
                }
            }

            if (const auto it = childPathKeysByParent.find(pathKey); it != childPathKeysByParent.end()) {
                for (const std::string& childKey : it->second) {
                    node.children.push_back(assembleMetadataNode(childKey,
                                                                  false,
                                                                  nodeIndexByPath,
                                                                  nodes,
                                                                  childPathKeysByParent,
                                                                  groupsByNodePath,
                                                                  defaultOptionsByNodePath,
                                                                  groupOptionsByKey,
                                                                  modelByKey,
                                                                  matchedOptions));
                }
            }

            return node;
        }

    } // namespace

    UiTree buildUiTree(ConfigModel& model, const ParsedMetadata& metadata) {
        if (!metadata.usable()) {
            return buildUiTree(model);
        }

        std::unordered_map<std::string, ConfigOption*> modelByKey;
        for (ConfigSection& section : model.sections) {
            for (ConfigOption& option : section.options) {
                modelByKey[fullOptionKey(option)] = &option;
            }
        }

        std::unordered_map<std::string, std::size_t> nodeIndexByPath;
        for (std::size_t i = 0; i < metadata.nodes.size(); ++i) {
            nodeIndexByPath[joinPath(metadata.nodes[i].path)] = i;
        }

        const std::string rootKey = joinPath({});
        if (nodeIndexByPath.find(rootKey) == nodeIndexByPath.end()) {
            // No root node was decoded (should not happen for well-formed metadata); nothing sane to
            // build the tree around, so fall back to the legacy heuristic rather than return an empty
            // tree.
            return buildUiTree(model);
        }

        std::unordered_map<std::string, std::vector<std::string>> childPathKeysByParent;
        for (const MetaNode& node : metadata.nodes) {
            if (node.path.empty()) {
                continue; // the root has no parent to link under
            }
            std::vector<std::string> parentPath = node.path;
            parentPath.pop_back();
            const std::string parentKey = joinPath(parentPath);
            if (nodeIndexByPath.find(parentKey) != nodeIndexByPath.end()) {
                childPathKeysByParent[parentKey].push_back(joinPath(node.path));
            }
            // A node whose parent path was never decoded is intentionally left unlinked here (rather
            // than guessed at): its own options still surface via the "Unmatched Options" fallback
            // bucket below instead of being silently dropped.
        }

        std::unordered_map<std::string, std::vector<const MetaGroup*>> groupsByNodePath;
        std::unordered_map<std::string, const MetaGroup*> groupLookup;
        for (const MetaGroup& group : metadata.groups) {
            const std::string nodeKey = joinPath(group.nodePath);
            groupLookup[nodeKey + groupKeySeparator + group.name] = &group;
            if (!isImplicitOptionsGroup(group) && !isNonpersistentOptionsGroup(group)) {
                groupsByNodePath[nodeKey].push_back(&group);
            }
        }

        std::unordered_map<std::string, std::vector<const MetaOption*>> defaultOptionsByNodePath;
        std::unordered_map<std::string, std::vector<const MetaOption*>> groupOptionsByKey;
        std::unordered_set<std::string> excludedOptionKeys; // nonpersistent-group options: hidden, not "unmatched"
        for (const MetaOption& option : metadata.options) {
            const std::string nodeKey = joinPath(option.nodePath);
            const std::string groupKey = nodeKey + groupKeySeparator + option.group;
            const auto groupIt = groupLookup.find(groupKey);
            const MetaGroup* group = groupIt != groupLookup.end() ? groupIt->second : nullptr;

            if (group != nullptr && isNonpersistentOptionsGroup(*group)) {
                excludedOptionKeys.insert(option.key);
            } else if (group != nullptr && !isImplicitOptionsGroup(*group)) {
                groupOptionsByKey[groupKey].push_back(&option);
            } else {
                defaultOptionsByNodePath[nodeKey].push_back(&option);
            }
        }

        UiTree tree;
        std::unordered_set<ConfigOption*> matchedOptions;
        tree.topLevel.push_back(assembleMetadataNode(rootKey,
                                                      true,
                                                      nodeIndexByPath,
                                                      metadata.nodes,
                                                      childPathKeysByParent,
                                                      groupsByNodePath,
                                                      defaultOptionsByNodePath,
                                                      groupOptionsByKey,
                                                      modelByKey,
                                                      matchedOptions));

        UiNode unmatched;
        unmatched.type = UiNodeType::Node;
        unmatched.kind = "unmatched";
        unmatched.kindSource = "snodec-control-fallback";
        unmatched.label = "Unmatched Options";
        for (ConfigSection& section : model.sections) {
            for (ConfigOption& option : section.options) {
                if (matchedOptions.find(&option) == matchedOptions.end() &&
                    excludedOptionKeys.find(fullOptionKey(option)) == excludedOptionKeys.end()) {
                    unmatched.children.push_back(makeOptionNode(&option));
                }
            }
        }
        if (!unmatched.children.empty()) {
            tree.topLevel.push_back(std::move(unmatched));
        }

        return tree;
    }

    namespace {

        void flattenRecursive(
            std::vector<UiNode>& nodes, int depth, std::optional<std::size_t> parentIndex, std::vector<FlatNode>& out) {
            for (UiNode& node : nodes) {
                const std::size_t myIndex = out.size();
                out.push_back(FlatNode{&node, depth, parentIndex});
                if (node.type != UiNodeType::Option && node.expanded) {
                    flattenRecursive(node.children, depth + 1, myIndex, out);
                }
            }
        }

    } // namespace

    std::vector<FlatNode> flattenVisibleNodes(UiTree& tree) {
        std::vector<FlatNode> out;
        flattenRecursive(tree.topLevel, 0, std::nullopt, out);
        return out;
    }

} // namespace snodec::control::ui
