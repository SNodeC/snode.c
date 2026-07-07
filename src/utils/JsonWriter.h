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

#ifndef UTILS_JSONWRITER_H
#define UTILS_JSONWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <ostream>
#include <string_view>
#include <utility>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    class JsonWriter {
    public:
        explicit JsonWriter(std::ostream& out);

        void beginObject();
        void endObject();

        void beginArray();
        void endArray();

        void key(std::string_view key);

        void string(std::string_view value);
        void number(std::string_view value);
        void boolean(bool value);
        void null();

    private:
        enum class Container { Object, Array };

        void commaIfNeeded();
        void beforeValue();
        void writeEscaped(std::string_view value);

        std::ostream& out;
        std::vector<std::pair<Container, bool>> stack;
        bool afterKey = false;
    };

} // namespace utils

#endif // UTILS_JSONWRITER_H
