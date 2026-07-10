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

#include "ConfigModel.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace snodec::control {

    ConfigSection& ConfigModel::sectionFor(const std::string& sectionName) {
        auto it = std::find_if(sections.begin(), sections.end(), [&sectionName](const ConfigSection& section) {
            return section.name == sectionName;
        });

        if (it != sections.end()) {
            return *it;
        }

        sections.push_back(ConfigSection{sectionName, {}});

        return sections.back();
    }

    std::size_t ConfigModel::sectionCount() const {
        return sections.size();
    }

    std::size_t ConfigModel::optionCount() const {
        std::size_t count = 0;

        for (const ConfigSection& section : sections) {
            count += section.options.size();
        }

        return count;
    }

    std::size_t ConfigModel::requiredOptionCount() const {
        std::size_t count = 0;

        for (const ConfigSection& section : sections) {
            for (const ConfigOption& option : section.options) {
                if (option.required) {
                    ++count;
                }
            }
        }

        return count;
    }

    std::size_t ConfigModel::activeOptionCount() const {
        std::size_t count = 0;

        for (const ConfigSection& section : sections) {
            for (const ConfigOption& option : section.options) {
                if (option.hasActiveValue) {
                    ++count;
                }
            }
        }

        return count;
    }

    std::pair<std::string, std::string> splitSectionAndKey(const std::string& fullyQualifiedName) {
        const std::size_t lastDot = fullyQualifiedName.find_last_of('.');

        if (lastDot == std::string::npos) {
            return {"", fullyQualifiedName};
        }

        return {fullyQualifiedName.substr(0, lastDot), fullyQualifiedName.substr(lastDot + 1)};
    }

    std::string fullOptionKey(const ConfigOption& option) {
        if (option.section.empty()) {
            return option.key;
        }

        return option.section + "." + option.key;
    }

    void applyMetadataToModel(ConfigModel& model, const ParsedMetadata& metadata) {
        if (!metadata.usable()) {
            return;
        }

        std::unordered_map<std::string, ConfigOption*> optionsByKey;
        for (ConfigSection& section : model.sections) {
            for (ConfigOption& option : section.options) {
                optionsByKey[fullOptionKey(option)] = &option;
            }
        }

        std::unordered_set<std::string> metadataKeys;
        for (const MetaOption& metaOption : metadata.options) {
            metadataKeys.insert(metaOption.key);

            const auto it = optionsByKey.find(metaOption.key);
            if (it == optionsByKey.end()) {
                continue;
            }
            ConfigOption& option = *it->second;

            if (!metaOption.description.empty()) {
                // Prefer metadata's description verbatim over the legacy line parser's reconstruction:
                // a description containing a "key = value"-shaped explanatory line (see the
                // fromCommentedDefaultLine reconciliation below) can otherwise end up truncated or
                // scrambled, since the legacy parser has no way to tell such a line apart from a real
                // commented default while accumulating description text.
                option.description = metaOption.description;
            }

            if (!metaOption.type.kind.empty()) {
                option.typeKind = metaOption.type.kind;
            }
            if (!metaOption.type.kindSource.empty()) {
                option.typeKindSource = metaOption.type.kindSource;
            }
            if (!metaOption.type.items.empty()) {
                option.typeItems = metaOption.type.items;
            }
            option.needs = metaOption.needs;
            option.excludes = metaOption.excludes;
            option.requiredEffective = metaOption.required.effective;
            if (!metaOption.required.source.empty()) {
                option.requiredSource = metaOption.required.source;
            }
            option.configFileWritable = metaOption.configFile.writable;
            option.configFileSection = metaOption.configFile.section;
        }

        // Structured metadata is authoritative for option identity. A parsed option that exists only
        // because a commented "#key=value"-shaped line was found - never a real, active/uncommented
        // line for that same key - and has no corresponding metadata entry is not a real option. This
        // happens when a line inside another option's multi-line description text itself happens to
        // look like "key = value" (e.g. "sni = SNI of the virtual server" inside --sni-cert's
        // description, which SNode.C legitimately emits as a plain comment continuation line, not a
        // commented default for a nonexistent "sni" option): the legacy line parser has no way to tell
        // that apart from a real commented default on its own, so this reconciliation step is what
        // actually prevents it from becoming a phantom "Unmatched Options" entry. An option backed by
        // at least one real active/uncommented line is always preserved, metadata match or not, since
        // it may be a genuine option a user set by hand that this particular metadata simply doesn't
        // describe.
        for (ConfigSection& section : model.sections) {
            std::vector<ConfigOption>& options = section.options;
            options.erase(std::remove_if(options.begin(),
                                         options.end(),
                                         [&metadataKeys](const ConfigOption& option) {
                                             return option.fromCommentedDefaultLine && !option.fromActiveLine &&
                                                    metadataKeys.find(fullOptionKey(option)) == metadataKeys.end();
                                         }),
                          options.end());
        }
    }

} // namespace snodec::control
