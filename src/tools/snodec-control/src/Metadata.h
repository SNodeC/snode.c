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

#include <string>
#include <vector>

namespace snodec::control {

    struct MetaNode {
        std::string kind;
        std::string kindSource;
        std::string displayName;
        std::vector<std::string> path;
        bool disabled = false;
    };

    struct MetaGroup {
        std::string name;
        std::string kind;
        std::vector<std::string> nodePath;
    };

    struct MetaType {
        std::string kind;
        std::string kindSource;
        std::string items;
    };

    struct MetaOption {
        std::string key;
        std::vector<std::string> nodePath;
        std::string group;
        std::string description;
        std::string requiredSource;
        MetaType type;
        std::vector<std::string> needs;
        std::vector<std::string> excludes;
    };

    // Parsed -S blocks. Presence distinguishes an absent format from a rejected metadata attempt.
    // Any unknown schema/version disables metadata use; warnings explain rejected blocks.
    struct ParsedMetadata {
        bool present = false;
        bool schemaRecognized = true;
        bool documentPresent = false;
        std::vector<MetaNode> nodes;
        std::vector<MetaGroup> groups;
        std::vector<MetaOption> options;
        std::vector<std::string> warnings;

        // True exactly when the metadata-native path should be used: metadata was present, its
        // schema/version were recognized, and a document block was recognized.
        bool usable() const;
    };

    // Decode JSON comments by their begin/end marker. Malformed blocks are dropped with a warning;
    // a malformed document or an unknown schema/version makes the metadata unusable.
    ParsedMetadata parseMetaBlocks(const std::string& text);

} // namespace snodec::control

#endif // SNODECCONTROL_METADATA_H
