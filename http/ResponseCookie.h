/* inclusion guard */
#ifndef __RESPINSECOOKIE_H__
#define __RESPINSECOOKIE_H__

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <ctime>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class ResponseCookieOptions {    
public:
    ResponseCookieOptions() {}
    ResponseCookieOptions& domain(const std::string& domain);
    ResponseCookieOptions& expire(const time_t& expires);
    ResponseCookieOptions& httpOnly(const bool httpOnly = false);
    ResponseCookieOptions& maxAge(const int& maxAge);
    ResponseCookieOptions& path(const std::string& path = "/"); 
    ResponseCookieOptions& secure(const bool secure = true);
    ResponseCookieOptions& sameSite(const std::string& sameSite = "Unspecified");

    const std::string toString() const;

private:
    std::string m_domain = "";
    std::string m_expires = "";
    bool m_httpOnly = false;
    int m_maxAge = 0;
    std::string m_path = "";
    bool m_secure = false;
    std::string m_sameSite = "";   
};

/*
public:
    ResponseCookieOptions(const std::string& domain = "",
                            const time_t& expires = time(NULL),
                            const bool httpOnly = false, 
                            const int& maxAge = 0, 
                            const std::string& path = "/", 
                            const bool secure = false, 
                            const std::string& sameSite = "Unspecified");
    
    void SetExpiryDate(const std::time_t& expires);
    
private:
    const std::string domain;
    std::string expires;
    const bool httpOnly;
    const int maxAge;
    const std::string path;
    const bool secure;
    const std::string sameSite; 
*/

class ResponseCookie {
public:
    ResponseCookie(const std::string& name, const std::string& value, ResponseCookieOptions& options) : name(name), value(value), options(options) {}
    const std::string toString() const;
    
protected:
    const std::string name;
    const std::string value;
    const ResponseCookieOptions& options;
    
    friend class HTTPContext;
};

#endif /* __RESPONSECOOKIE_H__ */
