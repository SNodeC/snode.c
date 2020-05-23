#include "ResponseCookie.h"

#include <time.h>

ResponseCookieOptions::ResponseCookieOptions(const std::string& domain, const time_t& expires, const bool httpOnly, const int& maxAge, const std::string& path, const bool secure, const std::string& sameSite) :
    domain(domain),
    httpOnly(httpOnly),
    maxAge(maxAge),
    path(path),
    secure(secure),
    sameSite(sameSite)
{
    if (expires == time(NULL)) {
        struct tm* tm = localtime(&expires);
        tm->tm_mday += 1;
        SetExpiryDateGMT(mktime(tm));
    } else {
        SetExpiryDateGMT(expires);
    }
};
        
void ResponseCookieOptions::SetExpiryDateGMT(const std::time_t& expires)
{
    char buffer[100];
    struct tm time = *gmtime(&expires);
    this->expires = std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %Z", &time);
}

    
