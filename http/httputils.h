#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace httputils {
    std::string url_decode(std::string text);

    std::string& str_trimm(std::string& text);
 
    std::pair<std::string, std::string> str_split(const std::string& base, char c_middle, char c_end = 0);
    
    std::string to_http_date(struct tm* tm = 0);
    
    struct tm from_http_date(std::string& http_date);
    
    std::string file_mod_http_date(std::string& filePath);
}
