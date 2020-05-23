/* inclusion guard */
#ifndef __RESPONSECOOKIE_H__
#define __RESPONSECOOKIE_H__

#include <ctime>
#include <string>

class ResponseCookieOptions {
public:
    // dropped encode, and signed because these are express only
    // set all the default values
    ResponseCookieOptions(const std::string& domain = "",
                            const time_t& expires = time(NULL),
                            const bool httpOnly = false,
                            const int& maxAge = 0,
                            const std::string& path = "/",
                            const bool secure = false,
                            const std::string& sameSite = "Unspecified");
    void SetExpiryDateGMT(const std::time_t& expires);

private:
    const std::string domain;
    std::string expires; // expiry date in GMT: "Wed, 09 Feb 1994 22:23:32 GMT"
    const bool httpOnly; // flags the cookie to be accessible only by the web server
    const int maxAge; // max age in milliseconds
    const std:: string path; // defaults to "/"
    const bool secure; // marks the cookie to be used with https only
    const std::string sameSite;
};


class ResponseCookie {
public:
    ResponseCookie(const std::string& value, const ResponseCookieOptions options) : value(value), options(options) {}
    
protected:
    const std::string value;
    const ResponseCookieOptions options;
    
    friend class HTTPContext;
};

#endif /* __RESPONSECOOKIE_H__ */
