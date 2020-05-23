#ifndef RESPONSECOOKIEOPTIONS_H
#define RESPONSECOOKIEOPTIONS_H

#include <ctime>
#include <string>

class ResponseCookieOptions {
public:
    // dropped encode, and signed because these are express only
    // set all the default values
    ResponseCookieOptions() {}
    ResponseCookieOptions& domain(const std::string& domain);
    ResponseCookieOptions& expires(const time_t& expires);
    ResponseCookieOptions& httpOnly(const bool httpOnly = true);
    ResponseCookieOptions& maxAge(const int& maxAge);
    ResponseCookieOptions& path(const std::string& path = "/");
    ResponseCookieOptions& secure(const bool secure = true);
    ResponseCookieOptions& sameSite(const std::string& sameSite = "Unspecified");
    
    const std::string toString() const;

private:
    std::string m_domain = "";
    std::string m_expires = ""; // expiry date in GMT: "Wed, 09 Feb 1994 22:23:32 GMT"
    bool m_httpOnly = false; // flags the cookie to be accessible only by the web server
    int m_maxAge = 0; // max age in milliseconds
    std::string m_path = ""; // defaults to "/"
    bool m_secure = false; // marks the cookie to be used with https only
    std::string m_sameSite = "";
};

#endif // RESPONSECOOKIEOPTIONS_H
