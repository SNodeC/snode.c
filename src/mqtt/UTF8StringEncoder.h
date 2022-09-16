/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef MQTT_UTF8STRINGENCODER_H
#define MQTT_UTF8STRINGENCODER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    class UTF8StringEncoder {
    public:
        UTF8StringEncoder(const std::string& string); // cppcheck-suppress noExplicitConstructor
        UTF8StringEncoder(const UTF8StringEncoder&) = default;
        UTF8StringEncoder(UTF8StringEncoder&&) = default;

        UTF8StringEncoder& operator=(const UTF8StringEncoder&) = default;
        UTF8StringEncoder& operator=(UTF8StringEncoder&&) = default;

        operator std::string();
        operator const std::string() const;

    private:
        std::string string;
    };

} // namespace mqtt

#endif // MQTT_UTF8STRINGENCODER_H
