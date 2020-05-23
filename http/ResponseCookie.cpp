#include "ResponseCookie.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <time.h>
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

ResponseCookieOptions& ResponseCookieOptions::domain(const std::string& domain){
    m_domain = domain;
    return *this;
}

ResponseCookieOptions& ResponseCookieOptions::expires(const time_t& expires) {
    char buffer[1000];
    struct tm time = *gmtime(&expires);
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %Z", &time);
    m_expires = buffer;
    return *this;
}

ResponseCookieOptions& ResponseCookieOptions::httpOnly(const bool httpOnly) {
    m_httpOnly = httpOnly;
    return *this;
}

ResponseCookieOptions& ResponseCookieOptions::maxAge(const int& maxAge) {
    m_maxAge = maxAge;
    return *this;
}

ResponseCookieOptions& ResponseCookieOptions::path(const std::string& path) {
    m_path = path;
    return *this;
}

ResponseCookieOptions& ResponseCookieOptions::secure(const bool secure) {
    m_secure = secure;
    return *this;
}

ResponseCookieOptions& ResponseCookieOptions::sameSite(const std::string& sameSite) {
    m_sameSite = sameSite;
    return *this;
}

const std::string ResponseCookieOptions::toString() const {
    std::string opt = "";
    opt += (m_domain == "") ? "" : "; Domain=" + m_domain;
    opt += (m_expires == "") ? "" : "; Expires=" + m_expires;
    opt += (m_httpOnly == false) ? "" : "; HttpOnly";
    opt += (m_maxAge == 0) ? "" : "; Max-Age=" + std::to_string(m_maxAge);
    opt += (m_path == "") ? "" : "; Path=" + m_path;
    opt += (m_secure == false) ? "" : "; Secure";
    opt += (m_sameSite == "") ? "" : " SameSite=" + m_sameSite;
    return opt;
}

const std::string ResponseCookie::toString() const {
    return name + "=" + value + options.toString(); // "cookiename=value" or "cookiename=value; Domain=domainvalue; Secure"
}
