/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "utils/system/time.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <iomanip>
#include <sstream>
#include <strings.h>
#include <sys/stat.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace httputils {

    static int from_hex(int ch) {
        return isdigit(ch) != 0 ? ch - '0' : tolower(ch) - 'a' + 10;
    }

    std::string url_decode(const std::string& text) {
        std::string escaped;

        for (std::string::const_iterator i = text.begin(), n = text.end(); i != n; ++i) {
            const std::string::value_type c = (*i);

            if (c == '%') {
                if ((i[1] != 0) && (i[2] != 0)) {
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

        for (const char c : text) {
            if ((isalnum(c) != 0) || c == '-' || c == '_' || c == '.' || c == '~') {
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

        const unsigned long middle = base.find_first_of(c_middle);

        split.first = base.substr(0, middle);

        if (middle != std::string::npos) {
            split.second = base.substr(middle + 1);
        }

        return split;
    }

    std::pair<std::string, std::string> str_split_last(const std::string& base, char c_middle) {
        std::pair<std::string, std::string> split;

        const unsigned long middle = base.find_last_of(c_middle);

        split.first = base.substr(0, middle);

        if (middle != std::string::npos) {
            split.second = base.substr(middle + 1);
        }

        return split;
    }

    std::string to_http_date(struct tm* tm) {
        char buf[100];

        if (tm == nullptr) {
            const time_t now = utils::system::time(nullptr);
            tm = utils::system::gmtime(&now);
        } else {
            const time_t time = utils::system::mktime(tm);
            tm = utils::system::gmtime(&time);
        }

        (void) strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", tm);
        errno = 0; // Errno can be modified by strftime even though there is no error

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
        errno = 0; // Errno can be modified by strftime even though there is no error

        return std::string(buf);
    }

    std::string::iterator to_lower(std::string& string) {
        return std::transform(string.begin(), string.end(), string.begin(), ::tolower);
    }

    bool ci_contains(const std::string& str1, const std::string& str2) {
        auto it = std::search(str1.begin(), str1.end(), str2.begin(), str2.end(), [](char ch1, char ch2) {
            return std::toupper(ch1) == std::toupper(ch2);
        });

        return (it != str1.end());
    }

    bool ci_equals(const std::string& str1, const std::string& str2) {
        return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(), [](char ch1, char ch2) {
            return std::toupper(ch1) == std::toupper(ch2);
        });
    }

    bool ciLess::operator()(const std::string& a, const std::string& b) const {
        return ::strcasecmp(a.c_str(), b.c_str()) < 0;
    }

} // namespace httputils
