/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef WEB_HTTP_COOKIEOPTIONS_H
#define WEB_HTTP_COOKIEOPTIONS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    class CookieOptions {
    public:
        CookieOptions(const std::string& value, const std::map<std::string, std::string>& options)
            : value(value)
            , options(options) {
        }

        explicit CookieOptions(const std::string& value)
            : value(value) {
        }

        void setOption(const std::string& optionName, const std::string& optionValue) {
            options[optionName] = optionValue;
        }

        const std::map<std::string, std::string>& getOptions() const {
            return options;
        }

        const std::string& getValue() const {
            return value;
        }

    private:
        std::string value;
        std::map<std::string, std::string> options;
    };

} // namespace web::http

#endif // WEB_HTTP_COOKIEOPTIONS_H
