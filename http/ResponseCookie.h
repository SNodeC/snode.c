#ifndef RESPONSECOOKIE_H
#define RESPONSECOOKIE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <ctime>
#include <time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @todo write docs
 */
class ResponseCookie
{
public:
    ResponseCookie(const std::string& name, const std::string& value, const struct CookieOptions& options) : name(name), value(value), options(options) {}
//     ResponseCookie(const std::string& name, const std::string& value) : name(name), value(value) {
//         options = CookieOptions{.maxAge=-1};
//     }
    std::string ToString();

protected:
    const std::string name;
    const std::string value;
    const struct CookieOptions& options;
    
    friend class HTTPContext;
};


enum SameSite { Lax, Strict, None, Undefined };


struct CookieOptions {
    std::string domain = "";
    time_t expires = 0;
    bool httpOnly = false;
    int maxAge = -1;
    std::string path = "";
    bool secure = false;
    SameSite sameSite = Undefined;
};





// class CookieOptions 
// {
// public:
//     
//     CookieOptions(const std::string& domain, const std::time_t& expires, const bool httpOnly, const int& maxAge, const std::string& path, const bool secure, const std::string& sameSite);
//     
// private:
//     
//     const std::string domain;
//     const time_t expires;
//     const bool httpOnly;
//     const int maxAge;
//     const std::string path;
//     const bool secure;
//     const std::string sameSite;
//     
//     time_t ConvertDateToGMT(time_t date);
// };

#endif // RESPONSECOOKIE_H
