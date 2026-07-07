/*
 * SNode.C - A Slim Toolkit for Network Communication
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

#include "utils/JsonWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cassert>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    JsonWriter::JsonWriter(std::ostream& out)
        : out(out) {
    }

    void JsonWriter::beginObject() {
        beforeValue();

        out << '{';
        stack.emplace_back(Container::Object, true);
    }

    void JsonWriter::endObject() {
        assert(!stack.empty());
        assert(stack.back().first == Container::Object);

        out << '}';
        stack.pop_back();
    }

    void JsonWriter::beginArray() {
        beforeValue();

        out << '[';
        stack.emplace_back(Container::Array, true);
    }

    void JsonWriter::endArray() {
        assert(!stack.empty());
        assert(stack.back().first == Container::Array);

        out << ']';
        stack.pop_back();
    }

    void JsonWriter::key(std::string_view value) {
        commaIfNeeded();
        writeEscaped(value);
        out << ':';
        afterKey = true;
    }

    void JsonWriter::string(std::string_view value) {
        beforeValue();
        writeEscaped(value);
    }

    void JsonWriter::number(std::string_view value) {
        beforeValue();
        out << value;
    }

    void JsonWriter::boolean(bool value) {
        beforeValue();
        out << (value ? "true" : "false");
    }

    void JsonWriter::null() {
        beforeValue();
        out << "null";
    }

    void JsonWriter::commaIfNeeded() {
        if (!stack.empty()) {
            if (!stack.back().second) {
                out << ',';
            }
            stack.back().second = false;
        }
    }

    void JsonWriter::beforeValue() {
        if (afterKey) {
            afterKey = false;
        } else {
            commaIfNeeded();
        }
    }

    void JsonWriter::writeEscaped(std::string_view value) {
        static constexpr char hex[] = "0123456789abcdef";

        out << '"';
        for (const unsigned char ch : value) {
            switch (ch) {
                case '"':
                    out << "\\\"";
                    break;
                case '\\':
                    out << "\\\\";
                    break;
                case '\b':
                    out << "\\b";
                    break;
                case '\f':
                    out << "\\f";
                    break;
                case '\n':
                    out << "\\n";
                    break;
                case '\r':
                    out << "\\r";
                    break;
                case '\t':
                    out << "\\t";
                    break;
                default:
                    if (ch < 0x20) {
                        out << "\\u00" << hex[(ch >> 4) & 0x0f] << hex[ch & 0x0f];
                    } else {
                        out << static_cast<char>(ch);
                    }
                    break;
            }
        }
        out << '"';
    }

} // namespace utils
