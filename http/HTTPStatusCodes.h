#ifndef HTTPRESPONSECODES_H
#define HTTPRESPONSECODES_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class HTTPStatusCode {
public:
    static std::string reason(int status) {
        return statusCode[status];
    }

private:
    static std::map<int, std::string> statusCode;
};

#endif // HTTPRESPONSECODES_H
