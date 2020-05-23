#include "ResponseCookie.h"

const std::string ResponseCookie::toString() const {
    return name + "=" + value + options.toString(); // "cookiename=value" or "cookiename=value; Domain=domainvalue; Secure"
}
