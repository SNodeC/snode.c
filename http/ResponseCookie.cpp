#include "ResponseCookie.h"


std::string ResponseCookie::ToString()
{
    std::string cookiestring = name + "=" + value;
    
    if (options.domain != "") {
        cookiestring += "; Domain=" + options.domain;
    }
    if (options.expires > 0) {
        char buffer[100];
        struct tm gmTime = *gmtime(&options.expires);
        std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M_%S %Z", &gmTime);
        std::string expireString = buffer;
        cookiestring += "; Expires=" + expireString;
    }
    if (options.httpOnly) {
        cookiestring += "; HttpOnly";
    }
    if (options.maxAge >= 0) {
        cookiestring += "; MaxAge=" + std::to_string(options.maxAge);
    }
    if (options.path != "") {
        cookiestring += "; Path=" + options.path;
    }
    if (options.secure) {
        cookiestring += "; Secure";
    }
    if (options.sameSite != Undefined) {
        std::string sameSite = options.sameSite == Lax ? "Lax" : options.sameSite == Strict ? "Strict" : "None";
        cookiestring += "; SameSite=" + sameSite;
    }
    
    return cookiestring;
}


// CookieOptions::CookieOptions(const std::string& domain, const std::time_t& expires, const bool httpOnly, const int& maxAge, const std::string& path, const bool secure, const std::string& sameSite) :
//     domain(domain),
//     expires(expires),
//     httpOnly(httpOnly),
//     maxAge(maxAge),
//     path(path),
//     secure(secure),
//     sameSite(sameSite)
// {
//     ConvertDateToGMT(expires);
// }
// 
// time_t CookieOptions::ConvertDateToGMT(time_t date)
// {
//     
// }
