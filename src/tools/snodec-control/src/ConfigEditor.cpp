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

#include <cstddef>
#include <unordered_map>
#include <utility>

namespace snodec::control {

    namespace {

        // Renders a value for display, making an empty string visibly distinct from "no value at all".
        std::string quoteIfEmpty(const std::string& value) {
            return value.empty() ? "\"\"" : value;
        }

        ConfigOption& createOption(ConfigModel& model, const std::string& key) {
            const auto [sectionName, optionKey] = splitSectionAndKey(key);
            ConfigSection& section = model.sectionFor(sectionName);

            ConfigOption option;
            option.section = sectionName;
            option.key = optionKey;
            section.options.push_back(std::move(option));

            return section.options.back();
        }

    } // namespace

    LookupResult findOption(ConfigModel& model, const std::string& key) {
        LookupResult result;

        for (ConfigSection& section : model.sections) {
            for (ConfigOption& option : section.options) {
                if (fullOptionKey(option) == key) {
                    result.status = LookupStatus::Found;
                    result.option = &option;
                    return result;
                }
            }
        }

        std::vector<ConfigOption*> matches;
        for (ConfigSection& section : model.sections) {
            for (ConfigOption& option : section.options) {
                if (option.key == key) {
                    matches.push_back(&option);
                }
            }
        }

        if (matches.size() == 1) {
            result.status = LookupStatus::Found;
            result.option = matches.front();
            return result;
        }

        if (matches.size() > 1) {
            result.status = LookupStatus::Ambiguous;
            for (ConfigOption* option : matches) {
                result.candidates.push_back(fullOptionKey(*option));
            }
            return result;
        }

        result.status = LookupStatus::NotFound;
        return result;
    }

    std::string effectiveValueDisplay(const ConfigOption& option) {
        if (option.hasActiveValue && option.activeValue.has_value()) {
            return quoteIfEmpty(*option.activeValue);
        }

        if (option.defaultValue.has_value()) {
            return quoteIfEmpty(*option.defaultValue);
        }

        return option.required ? "<REQUIRED>" : "<unset>";
    }

    bool parseSetArgument(const std::string& argument, std::string& key, std::string& value) {
        const std::size_t pos = argument.find('=');

        if (pos == std::string::npos || pos == 0) {
            return false;
        }

        key = argument.substr(0, pos);
        value = argument.substr(pos + 1);

        return true;
    }

    EditOutcome applyEdits(ConfigModel& model, const std::vector<EditOperation>& operations, bool allowNewOptions) {
        EditOutcome outcome;

        for (const EditOperation& operation : operations) {
            const LookupResult lookup = findOption(model, operation.key);

            if (lookup.status == LookupStatus::Ambiguous) {
                std::string candidateList;
                for (std::size_t i = 0; i < lookup.candidates.size(); ++i) {
                    candidateList += (i == 0 ? "" : ", ") + lookup.candidates[i];
                }
                outcome.errors.push_back({operation.key, "ambiguous option '" + operation.key + "', candidates: " + candidateList});
                continue;
            }

            ConfigOption* option = nullptr;

            if (lookup.status == LookupStatus::Found) {
                option = lookup.option;
            } else {
                if (!allowNewOptions) {
                    outcome.errors.push_back(
                        {operation.key, "unknown option '" + operation.key + "' (use --allow-new-options to create it)"});
                    continue;
                }
                option = &createOption(model, operation.key);
            }

            const std::string before = effectiveValueDisplay(*option);

            if (operation.kind == EditOperation::Kind::Set) {
                option->activeValue = operation.value;
                option->hasActiveValue = true;

                const std::string after = effectiveValueDisplay(*option);
                if (before != after) {
                    outcome.changes.push_back({fullOptionKey(*option), before, after});
                }
            } else {
                const bool hadActiveValue = option->hasActiveValue;
                option->hasActiveValue = false;
                option->activeValue = std::nullopt;

                const std::string after = hadActiveValue ? "<unset>" : effectiveValueDisplay(*option);
                if (before != after) {
                    outcome.changes.push_back({fullOptionKey(*option), before, after});
                }
            }
        }

        return outcome;
    }

    bool isOptionMissingRequired(const ConfigOption& option) {
        if (!option.required) {
            return false;
        }

        const bool hasUsableActive = option.hasActiveValue && option.activeValue.has_value() && *option.activeValue != "<REQUIRED>";
        const bool hasUsableDefault = option.defaultValue.has_value() && *option.defaultValue != "<REQUIRED>";

        return !hasUsableActive && !hasUsableDefault;
    }

    std::vector<const ConfigOption*> missingRequiredOptions(const ConfigModel& model) {
        std::vector<const ConfigOption*> missing;

        for (const ConfigSection& section : model.sections) {
            for (const ConfigOption& option : section.options) {
                if (isOptionMissingRequired(option)) {
                    missing.push_back(&option);
                }
            }
        }

        return missing;
    }

    std::vector<ChangeRecord> mergeChangeRecords(const std::vector<ChangeRecord>& earlier, const std::vector<ChangeRecord>& later) {
        std::vector<ChangeRecord> merged = earlier;
        std::unordered_map<std::string, std::size_t> indexByKey;
        for (std::size_t i = 0; i < merged.size(); ++i) {
            indexByKey[merged[i].fullKey] = i;
        }

        for (const ChangeRecord& change : later) {
            const auto it = indexByKey.find(change.fullKey);
            if (it != indexByKey.end()) {
                merged[it->second].after = change.after;
            } else {
                indexByKey[change.fullKey] = merged.size();
                merged.push_back(change);
            }
        }

        std::vector<ChangeRecord> result;
        result.reserve(merged.size());
        for (const ChangeRecord& change : merged) {
            if (change.before != change.after) {
                result.push_back(change);
            }
        }

        return result;
    }

} // namespace snodec::control
