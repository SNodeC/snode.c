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

#include "Json.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace snodec::control {

    JsonValue JsonValue::makeNull() {
        return JsonValue{};
    }

    JsonValue JsonValue::makeBool(bool value) {
        JsonValue json;
        json.valueType = Type::Bool;
        json.boolValue = value;
        return json;
    }

    JsonValue JsonValue::makeNumber(long long value) {
        JsonValue json;
        json.valueType = Type::Number;
        json.numberValue = value;
        return json;
    }

    JsonValue JsonValue::makeString(std::string value) {
        JsonValue json;
        json.valueType = Type::String;
        json.stringValue = std::move(value);
        return json;
    }

    JsonValue JsonValue::makeArray(std::vector<JsonValue> items) {
        JsonValue json;
        json.valueType = Type::Array;
        json.arrayValue = std::move(items);
        return json;
    }

    JsonValue JsonValue::makeObject(std::vector<std::pair<std::string, JsonValue>> members) {
        JsonValue json;
        json.valueType = Type::Object;
        json.objectValue = std::move(members);
        return json;
    }

    JsonValue::Type JsonValue::type() const {
        return valueType;
    }

    bool JsonValue::isNull() const {
        return valueType == Type::Null;
    }

    bool JsonValue::isString() const {
        return valueType == Type::String;
    }

    bool JsonValue::isObject() const {
        return valueType == Type::Object;
    }

    bool JsonValue::isArray() const {
        return valueType == Type::Array;
    }

    const JsonValue* JsonValue::find(const std::string& key) const {
        if (valueType != Type::Object) {
            return nullptr;
        }

        const auto it = std::find_if(objectValue.begin(), objectValue.end(), [&key](const auto& member) {
            return member.first == key;
        });

        return it != objectValue.end() ? &it->second : nullptr;
    }

    std::string JsonValue::asString(const std::string& fallback) const {
        return valueType == Type::String ? stringValue : fallback;
    }

    bool JsonValue::asBool(bool fallback) const {
        return valueType == Type::Bool ? boolValue : fallback;
    }

    long long JsonValue::asNumber(long long fallback) const {
        return valueType == Type::Number ? numberValue : fallback;
    }

    std::optional<std::string> JsonValue::asOptionalString() const {
        if (valueType == Type::Null) {
            return std::nullopt;
        }
        if (valueType == Type::String) {
            return stringValue;
        }
        return std::nullopt;
    }

    std::vector<std::string> JsonValue::asStringArray() const {
        std::vector<std::string> result;
        if (valueType != Type::Array) {
            return result;
        }
        result.reserve(arrayValue.size());
        for (const JsonValue& item : arrayValue) {
            if (item.isString()) {
                result.push_back(item.stringValue);
            }
        }
        return result;
    }

    const std::vector<JsonValue>& JsonValue::items() const {
        return arrayValue;
    }

    const std::vector<std::pair<std::string, JsonValue>>& JsonValue::members() const {
        return objectValue;
    }

    namespace {

        class Parser {
        public:
            explicit Parser(const std::string& text)
                : text(text) {
            }

            JsonParseResult parse() {
                JsonParseResult result;

                skipWhitespace();
                if (!parseValue(result.value, result.error)) {
                    result.ok = false;
                    return result;
                }
                skipWhitespace();

                if (pos != text.size()) {
                    result.error = errorAt("trailing content after JSON value");
                    result.ok = false;
                    return result;
                }

                result.ok = true;
                return result;
            }

        private:
            const std::string& text;
            std::size_t pos = 0;

            std::string errorAt(const std::string& message) const {
                return message + " (at offset " + std::to_string(pos) + ")";
            }

            void skipWhitespace() {
                while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n' || text[pos] == '\r')) {
                    ++pos;
                }
            }

            bool consumeLiteral(const char* literal) {
                const std::size_t length = std::string_view(literal).size();
                if (text.compare(pos, length, literal) == 0) {
                    pos += length;
                    return true;
                }
                return false;
            }

            bool parseValue(JsonValue& out, std::string& error) {
                if (pos >= text.size()) {
                    error = errorAt("unexpected end of input while expecting a value");
                    return false;
                }

                switch (text[pos]) {
                    case '{':
                        return parseObject(out, error);
                    case '[':
                        return parseArray(out, error);
                    case '"':
                        return parseStringValue(out, error);
                    case 't':
                        if (consumeLiteral("true")) {
                            out = JsonValue::makeBool(true);
                            return true;
                        }
                        error = errorAt("invalid literal, expected 'true'");
                        return false;
                    case 'f':
                        if (consumeLiteral("false")) {
                            out = JsonValue::makeBool(false);
                            return true;
                        }
                        error = errorAt("invalid literal, expected 'false'");
                        return false;
                    case 'n':
                        if (consumeLiteral("null")) {
                            out = JsonValue::makeNull();
                            return true;
                        }
                        error = errorAt("invalid literal, expected 'null'");
                        return false;
                    default:
                        if (text[pos] == '-' || (std::isdigit(static_cast<unsigned char>(text[pos])) != 0)) {
                            return parseNumber(out, error);
                        }
                        error = errorAt(std::string("unexpected character '") + text[pos] + "'");
                        return false;
                }
            }

            bool parseObject(JsonValue& out, std::string& error) {
                ++pos; // consume '{'
                std::vector<std::pair<std::string, JsonValue>> members;

                skipWhitespace();
                if (pos < text.size() && text[pos] == '}') {
                    ++pos;
                    out = JsonValue::makeObject(std::move(members));
                    return true;
                }

                while (true) {
                    skipWhitespace();
                    if (pos >= text.size() || text[pos] != '"') {
                        error = errorAt("expected a string key in object");
                        return false;
                    }

                    JsonValue keyValue;
                    if (!parseStringValue(keyValue, error)) {
                        return false;
                    }

                    skipWhitespace();
                    if (pos >= text.size() || text[pos] != ':') {
                        error = errorAt("expected ':' after object key");
                        return false;
                    }
                    ++pos;
                    skipWhitespace();

                    JsonValue memberValue;
                    if (!parseValue(memberValue, error)) {
                        return false;
                    }
                    members.emplace_back(keyValue.asString(), std::move(memberValue));

                    skipWhitespace();
                    if (pos < text.size() && text[pos] == ',') {
                        ++pos;
                        continue;
                    }
                    if (pos < text.size() && text[pos] == '}') {
                        ++pos;
                        break;
                    }
                    error = errorAt("expected ',' or '}' in object");
                    return false;
                }

                out = JsonValue::makeObject(std::move(members));
                return true;
            }

            bool parseArray(JsonValue& out, std::string& error) {
                ++pos; // consume '['
                std::vector<JsonValue> items;

                skipWhitespace();
                if (pos < text.size() && text[pos] == ']') {
                    ++pos;
                    out = JsonValue::makeArray(std::move(items));
                    return true;
                }

                while (true) {
                    skipWhitespace();

                    JsonValue itemValue;
                    if (!parseValue(itemValue, error)) {
                        return false;
                    }
                    items.push_back(std::move(itemValue));

                    skipWhitespace();
                    if (pos < text.size() && text[pos] == ',') {
                        ++pos;
                        continue;
                    }
                    if (pos < text.size() && text[pos] == ']') {
                        ++pos;
                        break;
                    }
                    error = errorAt("expected ',' or ']' in array");
                    return false;
                }

                out = JsonValue::makeArray(std::move(items));
                return true;
            }

            static void appendUtf8(std::string& out, unsigned int codepoint) {
                if (codepoint <= 0x7F) {
                    out.push_back(static_cast<char>(codepoint));
                } else if (codepoint <= 0x7FF) {
                    out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
                    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                } else {
                    out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                }
            }

            bool parseStringValue(JsonValue& out, std::string& error) {
                ++pos; // consume opening '"'
                std::string value;

                while (true) {
                    if (pos >= text.size()) {
                        error = errorAt("unterminated string");
                        return false;
                    }

                    const char ch = text[pos];
                    if (ch == '"') {
                        ++pos;
                        break;
                    }
                    if (ch == '\\') {
                        ++pos;
                        if (pos >= text.size()) {
                            error = errorAt("unterminated escape sequence");
                            return false;
                        }
                        const char escaped = text[pos];
                        switch (escaped) {
                            case '"':
                                value.push_back('"');
                                break;
                            case '\\':
                                value.push_back('\\');
                                break;
                            case '/':
                                value.push_back('/');
                                break;
                            case 'b':
                                value.push_back('\b');
                                break;
                            case 'f':
                                value.push_back('\f');
                                break;
                            case 'n':
                                value.push_back('\n');
                                break;
                            case 'r':
                                value.push_back('\r');
                                break;
                            case 't':
                                value.push_back('\t');
                                break;
                            case 'u': {
                                if (pos + 4 >= text.size()) {
                                    error = errorAt("truncated \\u escape");
                                    return false;
                                }
                                unsigned int codepoint = 0;
                                for (int i = 1; i <= 4; ++i) {
                                    const char hexDigit = text[pos + static_cast<std::size_t>(i)];
                                    codepoint <<= 4;
                                    if (hexDigit >= '0' && hexDigit <= '9') {
                                        codepoint |= static_cast<unsigned int>(hexDigit - '0');
                                    } else if (hexDigit >= 'a' && hexDigit <= 'f') {
                                        codepoint |= static_cast<unsigned int>(hexDigit - 'a' + 10);
                                    } else if (hexDigit >= 'A' && hexDigit <= 'F') {
                                        codepoint |= static_cast<unsigned int>(hexDigit - 'A' + 10);
                                    } else {
                                        error = errorAt("invalid \\u escape digit");
                                        return false;
                                    }
                                }
                                pos += 4;
                                appendUtf8(value, codepoint);
                                break;
                            }
                            default:
                                error = errorAt(std::string("invalid escape character '") + escaped + "'");
                                return false;
                        }
                        ++pos;
                    } else {
                        value.push_back(ch);
                        ++pos;
                    }
                }

                out = JsonValue::makeString(std::move(value));
                return true;
            }

            bool parseNumber(JsonValue& out, std::string& error) {
                const std::size_t start = pos;
                if (pos < text.size() && text[pos] == '-') {
                    ++pos;
                }
                if (pos >= text.size() || std::isdigit(static_cast<unsigned char>(text[pos])) == 0) {
                    error = errorAt("invalid number");
                    return false;
                }
                while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])) != 0) {
                    ++pos;
                }
                // This schema only ever emits small non-negative integers ("version": 1); fractional
                // or exponent forms are not part of it, so reject them explicitly rather than
                // silently truncating.
                if (pos < text.size() && (text[pos] == '.' || text[pos] == 'e' || text[pos] == 'E')) {
                    error = errorAt("non-integer numbers are not supported by this scoped parser");
                    return false;
                }

                out = JsonValue::makeNumber(std::stoll(text.substr(start, pos - start)));
                return true;
            }
        };

    } // namespace

    JsonParseResult parseJson(const std::string& text) {
        Parser parser(text);
        return parser.parse();
    }

} // namespace snodec::control
