#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace httputils {
std::string url_decode(std::string text);

std::string& str_trimm(std::string& text);

std::pair<std::string, std::string> str_split(const std::string& base, char c_middle);

std::pair<std::string, std::string> str_split_last(const std::string& base, char c_middle);

std::string str_substr_char(const std::string& string, const char c, std::string::size_type& pos);

std::string to_http_date(struct tm* tm = 0);

struct tm from_http_date(std::string& http_date);

std::string file_mod_http_date(std::string& filePath);

std::string::iterator to_lower(std::string& string);
}
