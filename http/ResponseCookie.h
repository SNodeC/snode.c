/* inclusion guard */
#ifndef __RESPONSECOOKIE_H__
#define __RESPONSECOOKIE_H__

#include <ctime>
#include <string>

# include "ResponseCookieOptions.h"

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
