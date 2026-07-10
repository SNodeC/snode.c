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

#ifndef SNODECCONTROL_CONFIGMODEL_H
#define SNODECCONTROL_CONFIGMODEL_H

#include "Metadata.h"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace snodec::control {

    // A single configuration option discovered from a target application's `-s`/`--show-config` output.
    struct ConfigOption {
        std::string section;
        std::string key;
        std::string description;
        std::optional<std::string> defaultValue;
        std::optional<std::string> activeValue;
        bool required = false;
        bool hasActiveValue = false;

        // Metadata enrichment (see applyMetadataToModel() in Metadata.h): nullopt/empty whenever the
        // target's output carried no "#@" metadata for this specific option (an older target, or a
        // metadata schema/version this build does not recognize). Never required for correctness -
        // every field above already has a value from the plain-INI parse alone - these only add
        // structure/typing/relations information the INI lines never carried.
        std::optional<std::string> typeKind;       // e.g. "boolean"/"integer"/"number"/"string"
        std::optional<std::string> typeKindSource;  // provenance, e.g. "heuristic-name" in schema v1; not a certainty
        std::optional<std::string> typeItems;       // "single" | "list"
        std::vector<std::string> needs;             // option names this option's metadata says it needs
        std::vector<std::string> excludes;          // option names this option's metadata says it excludes
        std::optional<bool> requiredEffective;       // metadata's "required.effective" (current CLI11 state only)
        std::optional<std::string> requiredSource;   // e.g. "cli11-current-state"
        std::optional<bool> configFileWritable;
        std::optional<std::string> configFileSection; // always nullopt in schema v1 (see docs/config-comment-metadata.md)

        // Provenance from the legacy line parser (ConfigParser.cpp), used by applyMetadataToModel() to
        // tell a real option apart from a stray "key = value"-shaped line inside another option's
        // multi-line description (e.g. a description explaining an option's sub-values, such as
        // "sni = SNI of the virtual server" inside --sni-cert's help text) that only *looks* like a
        // commented default. An option is set (never cleared) as each kind of line is encountered for
        // its key, so both flags can end up true for one option (e.g. a commented default later
        // overridden by an active line).
        bool fromActiveLine = false;          // discovered via an uncommented "key=value" line
        bool fromCommentedDefaultLine = false; // discovered via a commented "#key=value" (or "#key=<REQUIRED>") line
    };

    struct ConfigSection {
        std::string name;
        std::vector<ConfigOption> options;
    };

    class ConfigModel {
    public:
        ConfigSection& sectionFor(const std::string& sectionName);

        std::size_t sectionCount() const;
        std::size_t optionCount() const;
        std::size_t requiredOptionCount() const;
        std::size_t activeOptionCount() const;

        std::vector<ConfigSection> sections;
    };

    // Splits a fully qualified, possibly dotted, option name into its section prefix and its trailing
    // key, e.g. "echoserver.local.port" becomes section "echoserver.local" and key "port". Names
    // without a dot belong to the unnamed root/global section. Shared by the parser (splitting names as
    // discovered) and the editor (splitting names given via --set/--unset for newly created options).
    std::pair<std::string, std::string> splitSectionAndKey(const std::string& fullyQualifiedName);

    // The inverse of splitSectionAndKey: the fully qualified name an option is addressed by on the
    // command line, i.e. "<section>.<key>" or just "<key>" for options in the unnamed root/global
    // section.
    std::string fullOptionKey(const ConfigOption& option);

    // Enriches every ConfigOption already discovered in `model` with the corresponding MetaOption's
    // typing/relations/required-state fields (matched by `key`, which both share verbatim - see
    // docs/config-comment-metadata.md in the SNode.C tree). A no-op when `!metadata.usable()`. An
    // option present in `model` with no matching metadata entry is left with its enrichment fields at
    // their empty defaults, exactly as if metadata were entirely absent for that one option - this
    // can legitimately happen for one option even when metadata is usable overall (see
    // ParsedMetadata's per-entity degrade-gracefully policy in Metadata.h), and is not itself an
    // error.
    void applyMetadataToModel(ConfigModel& model, const ParsedMetadata& metadata);

} // namespace snodec::control

#endif // SNODECCONTROL_CONFIGMODEL_H
