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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include "utils/hexdump.h"
#include "utils/system/time.h"
#include "web/http/CiStringMap.h"
#include "web/http/CookieOptions.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <iomanip>
#include <sstream>
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
        escaped.fill('0'); // cppcheck-suppress ignoredReturnValue
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

    std::string toString(const std::string& method,
                         const std::string& url,
                         const std::string& version,
                         const web::http::CiStringMap<std::string>& queries,
                         const web::http::CiStringMap<std::string>& header,
                         const web::http::CiStringMap<std::string>& cookies,
                         const std::vector<char>& body) {
        const int prefixLength = 9;
        int keyLength = 0;

        for (const auto& [key, value] : queries) {
            keyLength = std::max(keyLength, static_cast<int>(key.size()));
        }
        for (const auto& [key, value] : header) {
            keyLength = std::max(keyLength, static_cast<int>(key.size()));
        }
        for (const auto& [key, value] : cookies) {
            keyLength = std::max(keyLength, static_cast<int>(key.size()));
        }

        std::stringstream requestStream;

        requestStream << std::setw(prefixLength) << "Request"
                      << ": " << std::setw(keyLength) << "Method"
                      << " : " << method << "\n";
        requestStream << std::setw(prefixLength) << ""
                      << ": " << std::setw(keyLength) << "Url"
                      << " : " << url << "\n";
        requestStream << std::setw(prefixLength) << ""
                      << ": " << std::setw(keyLength) << "Version"
                      << " : " << version << "\n";

        std::string prefix;

        if (!queries.empty()) {
            prefix = "Queries";
            for (const auto& [query, value] : queries) {
                requestStream << std::setw(prefixLength) << prefix << ": " << std::setw(keyLength) << query << " : " << value << "\n";
                prefix = "";
            }
        }

        if (!header.empty()) {
            prefix = "Header";
            for (const auto& [field, value] : header) {
                requestStream << std::setw(prefixLength) << prefix << ": " << std::setw(keyLength) << field << " : " << value << "\n";
                prefix = "";
            }
        }

        if (!cookies.empty()) {
            prefix = "Cookies";
            for (const auto& [cookie, value] : cookies) {
                requestStream << std::setw(prefixLength) << prefix << ": " << std::setw(keyLength) << cookie << " : " << value << "\n";
                prefix = "";
            }
        }

        if (!body.empty()) {
            prefix = "Body";
            requestStream << std::setw(prefixLength) << prefix << utils::hexDump(body, prefixLength) << "\n";
        }

        std::string string = requestStream.str();
        if (!string.empty()) {
            string.pop_back();
        }

        return string;
    }

    std::string toString(const std::string& version,
                         const std::string& statusCode,
                         const std::string& reason,
                         const web::http::CiStringMap<std::string>& header,
                         const web::http::CiStringMap<web::http::CookieOptions>& cookies,
                         const std::vector<char>& body) {
        const int prefixLength = 9;
        int keyLength = 0;

        for (const auto& [key, value] : header) {
            keyLength = std::max(keyLength, static_cast<int>(key.size()));
        }
        for (const auto& [key, value] : cookies) {
            keyLength = std::max(keyLength, static_cast<int>(key.size()));
        }

        std::stringstream requestStream;

        requestStream << std::setw(prefixLength) << "Response"
                      << ": " << std::setw(keyLength) << "Version"
                      << " : " << version << "\n";
        requestStream << std::setw(prefixLength) << ""
                      << ": " << std::setw(keyLength) << "Status"
                      << " : " << statusCode << "\n";
        requestStream << std::setw(prefixLength) << ""
                      << ": " << std::setw(keyLength) << "Reason"
                      << " : " << reason << "\n";

        std::string prefix;

        if (!header.empty()) {
            prefix = "Header";
            for (const auto& [field, value] : header) {
                requestStream << std::setw(prefixLength) << prefix << ": " << std::setw(keyLength) << field << " : " << value << "\n";
                prefix = "";
            }
        }

        if (!cookies.empty()) {
            prefix = "Cookies";
            for (const auto& [cookie, options] : cookies) {
                requestStream << std::setw(prefixLength) << prefix << ": " << std::setw(keyLength) << cookie << " : " << options.getValue()
                              << "\n";
                for (const auto& [optionKey, optionValue] : options.getOptions()) {
                    requestStream << std::setw(prefixLength) << ""
                                  << ":" << std::setw(keyLength) << ""
                                  << " : " << optionKey << "=" << optionValue << "\n";
                }
                prefix = "";
            }
        }

        if (!body.empty()) {
            prefix = "Body";
            requestStream << std::setw(prefixLength) << prefix << utils::hexDump(body, prefixLength) << "\n";
        }

        std::string string = requestStream.str();
        if (!string.empty()) {
            string.pop_back();
        }

        return string;
    }

} // namespace httputils
