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

#ifndef SNODECCONTROL_JSON_H
#define SNODECCONTROL_JSON_H

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace snodec::control {

    // A small, hand-rolled JSON value tree and parser, deliberately scoped to what SNode.C's own
    // "#@ snodec.meta ..." comment-metadata schema actually emits (see docs/config-comment-metadata.md
    // in the SNode.C tree, and src/utils/Formatter.cpp's writeMetaBlock()/writeDocumentMeta()/
    // writeNodeMeta()/writeGroupMeta()/writeOptionMeta()): objects, arrays, strings (with \" \\ \n \r
    // \t and \uXXXX escapes), booleans, null, and small non-negative integers (only "version" ever
    // uses a bare number). Not a general-purpose JSON library: no floating point, no exponents, no
    // surrogate-pair handling beyond passing \uXXXX through as UTF-8 for the BMP range, since the
    // schema this parses never emits any of those. This mirrors the emitter's own choice not to take
    // an external JSON dependency, applied to the consumer side.
    class JsonValue {
    public:
        enum class Type { Null, Bool, Number, String, Array, Object };

        JsonValue() = default;

        static JsonValue makeNull();
        static JsonValue makeBool(bool value);
        static JsonValue makeNumber(long long value);
        static JsonValue makeString(std::string value);
        static JsonValue makeArray(std::vector<JsonValue> items);
        static JsonValue makeObject(std::vector<std::pair<std::string, JsonValue>> members);

        Type type() const;
        bool isNull() const;
        bool isString() const;
        bool isObject() const;
        bool isArray() const;

        // Object member lookup; returns nullptr if this is not an object or the key is absent.
        const JsonValue* find(const std::string& key) const;

        // Best-effort scalar extraction: returns `fallback` if this value is not a string/bool/etc.
        // Used pervasively by the metadata decoder, which treats a missing or wrong-typed field as
        // "value not available" (and lets the caller decide whether that is fatal for the entity)
        // rather than throwing.
        std::string asString(const std::string& fallback = {}) const;
        bool asBool(bool fallback = false) const;
        long long asNumber(long long fallback = 0) const;
        std::optional<std::string> asOptionalString() const; // nullopt for JSON null, else asString("")
        std::vector<std::string> asStringArray() const;       // {} if not an array of strings
        const std::vector<JsonValue>& items() const;
        const std::vector<std::pair<std::string, JsonValue>>& members() const;

    private:
        Type valueType = Type::Null;
        bool boolValue = false;
        long long numberValue = 0;
        std::string stringValue;
        std::vector<JsonValue> arrayValue;
        std::vector<std::pair<std::string, JsonValue>> objectValue;
    };

    struct JsonParseResult {
        bool ok = false;
        JsonValue value;
        std::string error; // human-readable, includes a character offset when parsing fails
    };

    // Parses a single JSON document from `text`. Trailing whitespace after the value is tolerated;
    // trailing non-whitespace content is an error (this parser is used on one reconstructed "#@"
    // block's worth of text at a time, never a stream of concatenated documents).
    JsonParseResult parseJson(const std::string& text);

} // namespace snodec::control

#endif // SNODECCONTROL_JSON_H
