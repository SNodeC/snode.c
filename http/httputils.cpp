#include "httputils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <sstream>

#include <iostream>

namespace httputils {

static char from_hex(char ch) {
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}


std::string url_decode(std::string text) {
    char h;
    std::ostringstream escaped;
    escaped.fill('0');
    
    for (auto i = text.begin(), n = text.end(); i != n; ++i) {
        std::string::value_type c = (*i);
        
        if (c == '%') {
            if (i[1] && i[2]) {
                h = from_hex(i[1]) << 4 | from_hex(i[2]);
                escaped << h;
                i += 2;
            }
        } else if (c == '+') {
            escaped << ' ';
        } else {
            escaped << c;
        }
    }
    
    return escaped.str();
}


std::string& str_trimm(std::string& text) {
    text.erase(text.find_last_not_of(" \t") + 1);
    text.erase(0, text.find_first_not_of(" \t"));
    
    return text;
}


std::pair<std::string, std::string>  str_split(const std::string& base, char c_middle, char c_end) {
    std::pair<std::string, std::string>  split;
    
    int middle  = base.find_first_of(c_middle);
    int end = base.find_first_of(c_end, middle + 1);
    
    split.first = base.substr(0, middle);
    split.second = base.substr(middle + 1, end);
    
    return split;
}


std::string to_http_date(struct tm* tm) {
    char buf[100];
    
    if (tm == 0) {
        time_t now = time(0);
        tm = gmtime(&now);
    }
    
    std::cout << "Zone: " << tm->tm_zone << std::endl;
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", tm);
    
    return std::string(buf);
}


struct tm from_http_date(std::string& http_date) {
    struct tm tm;
    
    strptime(http_date.c_str(), "%a, %d %b %Y %H:%M:%S", &tm);
    tm.tm_zone = "GMT";
    
    return tm;
}


std::string file_mod_http_date(std::string& filePath) {
    char buf[100];
    
    struct stat attrib;
    stat(filePath.c_str(), &attrib);
    
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&(attrib.st_mtime)));
    
    return std::string(buf);    
}

}
