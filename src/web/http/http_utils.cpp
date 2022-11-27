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

#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/time.h"

#include <algorithm>
#include <cctype>
#include <iomanip> // std::setw
#include <sstream>
#include <sys/stat.h>
// IWYU pragma: no_include <bits/struct_stat.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace httputils {

    static int from_hex(int ch) {
        return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
    }

    std::string url_decode(const std::string& text) {
        std::string escaped;

        for (std::string::const_iterator i = text.begin(), n = text.end(); i != n; ++i) {
            std::string::value_type c = (*i);

            if (c == '%') {
                if (i[1] && i[2]) {
                    escaped += static_cast<char>(from_hex(i[1]) << 4 | from_hex(i[2]));
                    i += 2;
                }
            } else if (c == '+') {
                escaped += ' ';
            } else {
                escaped += c;
            }
        }

        return escaped;
    }

    std::string url_encode(const std::string& text) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (std::string::const_iterator i = text.begin(), n = text.end(); i != n; ++i) {
            std::string::value_type c = (*i);

            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else {
                escaped << std::uppercase;
                escaped << '%' << std::setw(2) << static_cast<unsigned char>(c);
                escaped << std::nouppercase;
            }
        }

        return escaped.str();
    }

    std::string& str_trimm(std::string& text) {
        text.erase(text.find_last_not_of(" \t") + 1);
        text.erase(0, text.find_first_not_of(" \t"));

        return text;
    }

    std::pair<std::string, std::string> str_split(const std::string& base, char c_middle) {
        std::pair<std::string, std::string> split;

        unsigned long middle = base.find_first_of(c_middle);

        split.first = base.substr(0, middle);

        if (middle != std::string::npos) {
            split.second = base.substr(middle + 1);
        }

        return split;
    }

    std::pair<std::string, std::string> str_split_last(const std::string& base, char c_middle) {
        std::pair<std::string, std::string> split;

        unsigned long middle = base.find_last_of(c_middle);

        split.first = base.substr(0, middle);

        if (middle != std::string::npos) {
            split.second = base.substr(middle + 1);
        }

        return split;
    }

    std::string str_substr_char(const std::string& string, char c, std::string::size_type* pos) {
        std::string::size_type rpos = string.find_first_of(c, *pos);

        std::string result{};

        if (rpos != std::string::npos) {
            result = string.substr(*pos, rpos - *pos);
            *pos = rpos + 1;
        }

        return result;
    }

    std::string to_http_date(struct tm* tm) {
        char buf[100];

        if (tm == nullptr) {
            time_t now = core::system::time(nullptr);
            tm = core::system::gmtime(&now);
        } else {
            time_t time = core::system::mktime(tm);
            tm = core::system::gmtime(&time);
        }

        (void) strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", tm);

        return std::string(buf);
    }

    struct tm from_http_date(const std::string& http_date) {
        struct tm tm {};

        strptime(http_date.c_str(), "%a, %d %b %Y %H:%M:%S", &tm);
        tm.tm_zone = "GMT";

        return tm;
    }

    std::string file_mod_http_date(const std::string& filePath) {
        char buf[100];

        struct stat attrib {};
        stat(filePath.c_str(), &attrib); // TODO: to core::system

        (void) strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&(attrib.st_mtime))); // TODO: to core::system

        return std::string(buf);
    }

    std::string::iterator to_lower(std::string& string) {
        return std::transform(string.begin(), string.end(), string.begin(), ::tolower);
    }

    bool ci_comp(const std::string& str1, const std::string& str2) {
        return str1.size() == str2.size() && std::equal(str1.begin(), str1.end(), str2.begin(), [](auto a, auto b) {
                   return std::tolower(a) == std::tolower(b);
               });
    }

    bool ci_contains(const std::string& str1, const std::string& str2) {
        auto it = std::search(str1.begin(), str1.end(), str2.begin(), str2.end(), [](char ch1, char ch2) {
            return std::toupper(ch1) == std::toupper(ch2);
        });

        return (it != str1.end());
    }

} // namespace httputils
