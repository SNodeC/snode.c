#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cctype>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "httputils.h"


namespace httputils {

    static int from_hex(int ch) {
        return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
    }


    std::string url_decode(const std::string& text) {
        int h;

        std::string escaped;

        for (std::string::const_iterator i = text.begin(), n = text.end(); i != n; ++i) {
            std::string::value_type c = (*i);

            if (c == '%') {
                if (i[1] && i[2]) {
                    h = from_hex(i[1]) << 4 | from_hex(i[2]);
                    escaped += static_cast<char>(h);
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


    std::string str_trimm(const std::string& text) {
        std::string tmpText = text;
        tmpText.erase(text.find_last_not_of(" \t") + 1);
        tmpText.erase(0, text.find_first_not_of(" \t"));

        return tmpText;
    }


    std::pair<std::string, std::string> str_split(const std::string& base, char c_middle) {
        std::pair<std::string, std::string> split;

        int middle = base.find_first_of(c_middle);

        split.first = base.substr(0, middle);

        if (middle != std::string::npos) {
            split.second = base.substr(middle + 1);
        }

        return split;
    }


    std::pair<std::string, std::string> str_split_last(const std::string& base, char c_middle) {
        std::pair<std::string, std::string> split;

        int middle = base.find_last_of(c_middle);

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
            time_t now = time(nullptr);
            tm = gmtime(&now);
        } else {
            time_t time = mktime(tm);
            tm = gmtime(&time);
        }

        strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", tm);

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
        stat(filePath.c_str(), &attrib);

        strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&(attrib.st_mtime)));

        return std::string(buf);
    }


    std::string::iterator to_lower(std::string* string) {
        return std::transform(string->begin(), string->end(), string->begin(), ::tolower);
    }

} // namespace httputils
