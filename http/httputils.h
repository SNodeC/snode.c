#include <string>
#include <utility>

namespace httputils {
    std::string url_decode(std::string text);

    std::string& str_trimm(std::string& text);
    /*
    struct splitted {
        std::string first;
        std::string second;
    };*/
    
    std::pair<std::string, std::string> str_split(const std::string& base, char c_middle, char c_end = 0);
}
