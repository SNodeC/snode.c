#ifndef HTTPUTILS_H
#define HTTPUTILS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <bits/types/struct_tm.h> // for tm
#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace httputils {

    std::string url_decode(const std::string& text);

    std::string& str_trimm(std::string& text);

    std::pair<std::string, std::string> str_split(const std::string& base, char c_middle);

    std::pair<std::string, std::string> str_split_last(const std::string& base, char c_middle);

    std::string str_substr_char(const std::string& string, char c, std::string::size_type* pos);

    std::string to_http_date(struct tm* tm = nullptr);

    struct tm from_http_date(const std::string& http_date);

    std::string file_mod_http_date(const std::string& filePath);

    std::string::iterator to_lower(std::string& string);

} // namespace httputils

#endif // HTTPUTILS_H
