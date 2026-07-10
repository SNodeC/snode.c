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

#ifndef SNODECCONTROL_METADATA_H
#define SNODECCONTROL_METADATA_H

#include <optional>
#include <string>
#include <vector>

namespace snodec::control {

    // Mirrors the "required": { "effective": ..., "source": ... } object emitted for both "node"
    // and "option" entities. Per docs/config-comment-metadata.md this is *current CLI11 state only*,
    // not a canonical/registration-time value and not suppression-aware - snodec-control must not
    // treat it as more authoritative than that.
    struct MetaRequired {
        bool effective = false;
        std::string source;
    };

    struct MetaDocument {
        std::string schema;
        int version = 0;
        std::string applicationName;
        std::string applicationDescription;
        std::string scope;
        std::string syntax;
    };

    struct MetaNode {
        std::string kind;
        std::string kindSource;
        std::string name;
        std::string displayName;
        std::vector<std::string> path;
        std::string configPrefix;
        std::string group;
        std::string description;
        bool configurable = false;
        MetaRequired required;
        bool disabled = false;
    };

    struct MetaGroup {
        std::string name;
        std::string kind;
        std::string kindSource;
        std::vector<std::string> nodePath;
    };

    struct MetaCommandLine {
        std::optional<std::string> longName;
        std::optional<std::string> shortName;
        bool takesValue = false;
        bool repeatable = false;
    };

    struct MetaConfigFile {
        std::string key;
        std::optional<std::string> section;
        bool writable = true;
    };

    struct MetaType {
        std::string name;
        std::string kind;       // e.g. "boolean" | "integer" | "number" | "string" - heuristic, see kindSource
        std::string kindSource; // always "heuristic-name" in schema v1 (see docs); surface honestly, don't trust blindly
        std::string items;      // "single" | "list"
    };

    struct MetaValue {
        std::optional<std::string> cppDefault;
        std::optional<std::string> configured;
        std::string effective;
        std::string source;
        bool isCppDefault = false;
        bool isExplicitlyConfigured = false;
        bool isMissingRequired = false;
        std::optional<std::string> cppDefaultLiteral;
        std::optional<std::string> configuredLiteral;
        std::optional<std::string> effectiveLiteral;
    };

    struct MetaOption {
        std::string id;
        std::string key; // matches the literal INI key snodec-control's legacy parser already discovers
        std::string localName;
        std::string displayName;
        std::vector<std::string> nodePath;
        std::string group;
        std::string description;
        bool configurable = true;
        MetaRequired required;
        bool persistent = false;
        MetaCommandLine commandLine;
        MetaConfigFile configFile;
        MetaType type;
        std::vector<std::string> needs;
        std::vector<std::string> excludes;
        MetaValue value;
    };

    // The result of scanning one `-s`/`--show-config` capture for "#@ snodec.meta ..." blocks.
    //
    // `present` is true the moment any well-formed "snodec.meta begin/end" marker pair was found at
    // all, regardless of whether the schema turned out to be recognized - it answers "did this
    // target attempt to emit metadata", which is what callers need to distinguish "legacy target,
    // metadata absent by design" from "modern target, but something about the metadata was
    // unusable" (the latter still warrants a warning even though both fall back to legacy parsing).
    //
    // `schemaRecognized` is false when the "document" block's schema/version could not be decoded or
    // named a schema/version this build does not understand; callers must then fall back to legacy
    // parsing entirely rather than trust any partially-decoded node/group/option entries, since an
    // unrecognized schema version may have changed field meanings, not just added fields.
    struct ParsedMetadata {
        bool present = false;
        bool schemaRecognized = true;
        std::optional<MetaDocument> document;
        std::vector<MetaNode> nodes;
        std::vector<MetaGroup> groups;
        std::vector<MetaOption> options;
        std::vector<std::string> warnings;

        // True exactly when the metadata-native path should be used: metadata was present, its
        // schema/version were recognized, and at least the root node was decoded.
        bool usable() const;
    };

    // Scans `text` (the full `-s`/`--show-config` capture) for "#@ snodec.meta begin/end <entity>"
    // blocks, decodes each block's JSON body, and classifies it by its "entity" field. A single
    // malformed or unrecognized block is dropped with a warning and does not abort the scan of the
    // rest of the input (Step 2's "degrade gracefully per entity" requirement); an unrecognized
    // schema/version on the "document" block instead sets `schemaRecognized = false` for the whole
    // result, since later entities cannot be trusted to mean what this build assumes they mean.
    ParsedMetadata parseMetaBlocks(const std::string& text);

} // namespace snodec::control

#endif // SNODECCONTROL_METADATA_H
