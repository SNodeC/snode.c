/* inclusion guard */
#ifndef __RESPONSECOOKIE_H__
#define __RESPONSECOOKIE_H__

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <ctime>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

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
    
    
class ResponseCookie {

public:
    ResponseCookie(const std::string& name, const std::string& value, const ResponseCookieOptions& options) : name(name), value(value), options(options) {}
    const std::string toString() const;
    
protected:
    const std::string name;
    const std::string value;
    const ResponseCookieOptions& options;
    
    friend class HTTPContext;
};

#endif /* __RESPONSECOOKIE_H__ */
