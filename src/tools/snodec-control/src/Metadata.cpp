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

#include "Metadata.h"

#include "Json.h"

#include <sstream>

namespace snodec::control {

    namespace {

        constexpr const char* expectedSchema = "snodec.config.comment-meta";
        constexpr int expectedVersion = 1;

        struct RawBlock {
            std::string entity;
            std::string jsonText;
            std::size_t beginLine = 0;
        };

        // Strips exactly the "#@ " (or, leniently, "#@" alone) prefix the emitter always writes for
        // every line of a metadata block, reconstructing the original (unprefixed) line.
        std::string stripMetaPrefix(const std::string& line) {
            std::string rest = line.substr(2); // drop "#@"
            if (!rest.empty() && rest.front() == ' ') {
                rest.erase(0, 1);
            }
            return rest;
        }

        bool isMetaLine(const std::string& line) {
            return line.size() >= 2 && line[0] == '#' && line[1] == '@';
        }

        std::optional<std::string> matchBeginMarker(const std::string& content) {
            constexpr const char* beginPrefix = "snodec.meta begin ";
            if (content.rfind(beginPrefix, 0) == 0) {
                return content.substr(std::string(beginPrefix).size());
            }
            return std::nullopt;
        }

        std::optional<std::string> matchEndMarker(const std::string& content) {
            constexpr const char* endPrefix = "snodec.meta end ";
            if (content.rfind(endPrefix, 0) == 0) {
                return content.substr(std::string(endPrefix).size());
            }
            return std::nullopt;
        }

        // Scans the whole capture line by line and reconstructs each "#@ snodec.meta begin/end
        // <entity>" block's JSON body as plain text (one JSON document's worth of lines, "#@ "
        // stripped). A block whose "begin"/"end" entity names disagree, or that runs into
        // end-of-input without a matching "end" marker, is dropped with a warning - the scanner
        // still resumes at the next line rather than aborting the whole scan.
        std::vector<RawBlock> extractRawBlocks(const std::string& text, std::vector<std::string>& warnings) {
            std::vector<RawBlock> blocks;

            std::istringstream lines(text);
            std::string rawLine;
            std::size_t lineNumber = 0;

            std::optional<std::string> currentEntity;
            std::size_t currentBeginLine = 0;
            std::ostringstream currentJson;

            while (std::getline(lines, rawLine)) {
                ++lineNumber;

                if (!isMetaLine(rawLine)) {
                    if (currentEntity.has_value()) {
                        warnings.push_back("Line " + std::to_string(currentBeginLine) + ": '#@ snodec.meta begin " + *currentEntity +
                                           "' block never closed with a matching end marker; block ignored");
                        currentEntity.reset();
                    }
                    continue;
                }

                const std::string content = stripMetaPrefix(rawLine);

                if (const auto beginEntity = matchBeginMarker(content)) {
                    if (currentEntity.has_value()) {
                        warnings.push_back("Line " + std::to_string(currentBeginLine) + ": '#@ snodec.meta begin " + *currentEntity +
                                           "' block never closed before a new block began at line " + std::to_string(lineNumber) +
                                           "; block ignored");
                    }
                    currentEntity = *beginEntity;
                    currentBeginLine = lineNumber;
                    currentJson.str("");
                    currentJson.clear();
                    continue;
                }

                if (const auto endEntity = matchEndMarker(content)) {
                    if (!currentEntity.has_value()) {
                        warnings.push_back("Line " + std::to_string(lineNumber) + ": '#@ snodec.meta end " + *endEntity +
                                           "' with no matching begin marker; ignored");
                        continue;
                    }
                    if (*endEntity != *currentEntity) {
                        warnings.push_back("Line " + std::to_string(lineNumber) + ": '#@ snodec.meta end " + *endEntity +
                                           "' does not match '#@ snodec.meta begin " + *currentEntity + "' at line " +
                                           std::to_string(currentBeginLine) + "; block ignored");
                        currentEntity.reset();
                        continue;
                    }

                    blocks.push_back(RawBlock{*currentEntity, currentJson.str(), currentBeginLine});
                    currentEntity.reset();
                    continue;
                }

                if (currentEntity.has_value()) {
                    currentJson << content << '\n';
                } else {
                    warnings.push_back("Line " + std::to_string(lineNumber) +
                                       ": '#@' line outside of any 'snodec.meta begin/end' block; ignored: " + content);
                }
            }

            if (currentEntity.has_value()) {
                warnings.push_back("Line " + std::to_string(currentBeginLine) + ": '#@ snodec.meta begin " + *currentEntity +
                                   "' block never closed before end of input; block ignored");
            }

            return blocks;
        }

        MetaRequired decodeRequired(const JsonValue* json) {
            MetaRequired required;
            if (json == nullptr || !json->isObject()) {
                return required;
            }
            if (const JsonValue* effective = json->find("effective")) {
                required.effective = effective->asBool(false);
            }
            if (const JsonValue* source = json->find("source")) {
                required.source = source->asString();
            }
            return required;
        }

        MetaDocument decodeDocument(const JsonValue& json) {
            MetaDocument document;
            if (const JsonValue* schema = json.find("schema")) {
                document.schema = schema->asString();
            }
            if (const JsonValue* version = json.find("version")) {
                document.version = static_cast<int>(version->asNumber(0));
            }
            if (const JsonValue* application = json.find("application")) {
                if (const JsonValue* name = application->find("name")) {
                    document.applicationName = name->asString();
                }
                if (const JsonValue* description = application->find("description")) {
                    document.applicationDescription = description->asString();
                }
            }
            if (const JsonValue* scope = json.find("scope")) {
                document.scope = scope->asString();
            }
            if (const JsonValue* syntax = json.find("syntax")) {
                document.syntax = syntax->asString();
            }
            return document;
        }

        MetaNode decodeNode(const JsonValue& json) {
            MetaNode node;
            node.kind = json.find("kind") != nullptr ? json.find("kind")->asString() : std::string{};
            node.kindSource = json.find("kindSource") != nullptr ? json.find("kindSource")->asString() : std::string{};
            node.name = json.find("name") != nullptr ? json.find("name")->asString() : std::string{};
            node.displayName = json.find("displayName") != nullptr ? json.find("displayName")->asString() : std::string{};
            if (const JsonValue* path = json.find("path")) {
                node.path = path->asStringArray();
            }
            node.configPrefix = json.find("configPrefix") != nullptr ? json.find("configPrefix")->asString() : std::string{};
            node.group = json.find("group") != nullptr ? json.find("group")->asString() : std::string{};
            node.description = json.find("description") != nullptr ? json.find("description")->asString() : std::string{};
            if (const JsonValue* configurable = json.find("configurable")) {
                node.configurable = configurable->asBool(false);
            }
            node.required = decodeRequired(json.find("required"));
            if (const JsonValue* disabled = json.find("disabled")) {
                node.disabled = disabled->asBool(false);
            }
            return node;
        }

        MetaGroup decodeGroup(const JsonValue& json) {
            MetaGroup group;
            group.name = json.find("name") != nullptr ? json.find("name")->asString() : std::string{};
            group.kind = json.find("kind") != nullptr ? json.find("kind")->asString() : std::string{};
            group.kindSource = json.find("kindSource") != nullptr ? json.find("kindSource")->asString() : std::string{};
            if (const JsonValue* nodePath = json.find("nodePath")) {
                group.nodePath = nodePath->asStringArray();
            }
            return group;
        }

        MetaCommandLine decodeCommandLine(const JsonValue* json) {
            MetaCommandLine commandLine;
            if (json == nullptr || !json->isObject()) {
                return commandLine;
            }
            if (const JsonValue* longName = json->find("long")) {
                commandLine.longName = longName->asOptionalString();
            }
            if (const JsonValue* shortName = json->find("short")) {
                commandLine.shortName = shortName->asOptionalString();
            }
            if (const JsonValue* takesValue = json->find("takesValue")) {
                commandLine.takesValue = takesValue->asBool(false);
            }
            if (const JsonValue* repeatable = json->find("repeatable")) {
                commandLine.repeatable = repeatable->asBool(false);
            }
            return commandLine;
        }

        MetaConfigFile decodeConfigFile(const JsonValue* json) {
            MetaConfigFile configFile;
            if (json == nullptr || !json->isObject()) {
                return configFile;
            }
            if (const JsonValue* key = json->find("key")) {
                configFile.key = key->asString();
            }
            if (const JsonValue* section = json->find("section")) {
                configFile.section = section->asOptionalString();
            }
            if (const JsonValue* writable = json->find("writable")) {
                configFile.writable = writable->asBool(true);
            }
            return configFile;
        }

        MetaType decodeType(const JsonValue* json) {
            MetaType type;
            if (json == nullptr || !json->isObject()) {
                return type;
            }
            if (const JsonValue* name = json->find("name")) {
                type.name = name->asString();
            }
            if (const JsonValue* kind = json->find("kind")) {
                type.kind = kind->asString();
            }
            if (const JsonValue* kindSource = json->find("kindSource")) {
                type.kindSource = kindSource->asString();
            }
            if (const JsonValue* items = json->find("items")) {
                type.items = items->asString();
            }
            return type;
        }

        MetaValue decodeValue(const JsonValue* json) {
            MetaValue value;
            if (json == nullptr || !json->isObject()) {
                return value;
            }
            if (const JsonValue* cppDefault = json->find("cppDefault")) {
                value.cppDefault = cppDefault->asOptionalString();
            }
            if (const JsonValue* configured = json->find("configured")) {
                value.configured = configured->asOptionalString();
            }
            if (const JsonValue* effective = json->find("effective")) {
                value.effective = effective->asString();
            }
            if (const JsonValue* source = json->find("source")) {
                value.source = source->asString();
            }
            if (const JsonValue* isCppDefault = json->find("isCppDefault")) {
                value.isCppDefault = isCppDefault->asBool(false);
            }
            if (const JsonValue* isExplicitlyConfigured = json->find("isExplicitlyConfigured")) {
                value.isExplicitlyConfigured = isExplicitlyConfigured->asBool(false);
            }
            if (const JsonValue* isMissingRequired = json->find("isMissingRequired")) {
                value.isMissingRequired = isMissingRequired->asBool(false);
            }
            if (const JsonValue* cppDefaultLiteral = json->find("cppDefaultLiteral")) {
                value.cppDefaultLiteral = cppDefaultLiteral->asOptionalString();
            }
            if (const JsonValue* configuredLiteral = json->find("configuredLiteral")) {
                value.configuredLiteral = configuredLiteral->asOptionalString();
            }
            if (const JsonValue* effectiveLiteral = json->find("effectiveLiteral")) {
                value.effectiveLiteral = effectiveLiteral->asOptionalString();
            }
            return value;
        }

        // Returns nullopt (dropping the whole option, with `warnings` explaining why) only when the
        // block is missing the one field snodec-control cannot do anything useful without: the
        // config-file "key" used to match this metadata back to the option the legacy line parser
        // already discovered. Every other field degrades to its struct default instead of aborting.
        std::optional<MetaOption> decodeOption(const JsonValue& json, std::size_t beginLine, std::vector<std::string>& warnings) {
            MetaOption option;
            option.key = json.find("key") != nullptr ? json.find("key")->asString() : std::string{};
            if (option.key.empty()) {
                warnings.push_back("Line " + std::to_string(beginLine) + ": option metadata block has no usable 'key'; dropped");
                return std::nullopt;
            }

            option.id = json.find("id") != nullptr ? json.find("id")->asString() : option.key;
            option.localName = json.find("localName") != nullptr ? json.find("localName")->asString() : std::string{};
            option.displayName = json.find("displayName") != nullptr ? json.find("displayName")->asString() : option.localName;
            if (const JsonValue* nodePath = json.find("nodePath")) {
                option.nodePath = nodePath->asStringArray();
            }
            option.group = json.find("group") != nullptr ? json.find("group")->asString() : std::string{};
            option.description = json.find("description") != nullptr ? json.find("description")->asString() : std::string{};
            if (const JsonValue* configurable = json.find("configurable")) {
                option.configurable = configurable->asBool(true);
            }
            option.required = decodeRequired(json.find("required"));
            if (const JsonValue* persistent = json.find("persistent")) {
                option.persistent = persistent->asBool(false);
            }
            option.commandLine = decodeCommandLine(json.find("commandLine"));
            option.configFile = decodeConfigFile(json.find("configFile"));
            option.type = decodeType(json.find("type"));
            if (const JsonValue* relations = json.find("relations")) {
                if (const JsonValue* needs = relations->find("needs")) {
                    option.needs = needs->asStringArray();
                }
                if (const JsonValue* excludes = relations->find("excludes")) {
                    option.excludes = excludes->asStringArray();
                }
            }
            option.value = decodeValue(json.find("value"));

            return option;
        }

    } // namespace

    bool ParsedMetadata::usable() const {
        return present && schemaRecognized && document.has_value();
    }

    ParsedMetadata parseMetaBlocks(const std::string& text) {
        ParsedMetadata result;

        const std::vector<RawBlock> blocks = extractRawBlocks(text, result.warnings);
        if (blocks.empty()) {
            return result; // present == false: no target-side attempt at metadata at all
        }
        result.present = true;

        for (const RawBlock& block : blocks) {
            const JsonParseResult parsed = parseJson(block.jsonText);
            if (!parsed.ok) {
                result.warnings.push_back("Line " + std::to_string(block.beginLine) + ": malformed JSON in '#@ snodec.meta begin " +
                                          block.entity + "' block: " + parsed.error + "; block dropped");
                if (block.entity == "document") {
                    result.schemaRecognized = false;
                }
                continue;
            }
            if (!parsed.value.isObject()) {
                result.warnings.push_back("Line " + std::to_string(block.beginLine) + ": '#@ snodec.meta begin " + block.entity +
                                          "' block is not a JSON object; dropped");
                if (block.entity == "document") {
                    result.schemaRecognized = false;
                }
                continue;
            }

            const JsonValue* schemaField = parsed.value.find("schema");
            const JsonValue* versionField = parsed.value.find("version");
            const std::string schema = schemaField != nullptr ? schemaField->asString() : std::string{};
            const int version = versionField != nullptr ? static_cast<int>(versionField->asNumber(0)) : 0;
            if (schema != expectedSchema || version != expectedVersion) {
                result.warnings.push_back("Line " + std::to_string(block.beginLine) + ": unrecognized metadata schema/version ('" +
                                          schema + "' v" + std::to_string(version) + "'); falling back to legacy parsing");
                result.schemaRecognized = false;
                continue;
            }

            if (block.entity == "document") {
                result.document = decodeDocument(parsed.value);
            } else if (block.entity == "node") {
                result.nodes.push_back(decodeNode(parsed.value));
            } else if (block.entity == "group") {
                result.groups.push_back(decodeGroup(parsed.value));
            } else if (block.entity == "option") {
                if (auto option = decodeOption(parsed.value, block.beginLine, result.warnings)) {
                    result.options.push_back(std::move(*option));
                }
            } else {
                result.warnings.push_back("Line " + std::to_string(block.beginLine) + ": unrecognized metadata entity '" + block.entity +
                                          "'; block ignored (forward-compatible: a future schema may define new entity types)");
            }
        }

        return result;
    }

} // namespace snodec::control
