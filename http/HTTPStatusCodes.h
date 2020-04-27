#ifndef HTTPRESPONSECODES_H
#define HTTPRESPONSECODES_H

#include <map>


class HTTPStatusCode
{
public:
    static std::string reason(int status) {
        return statusCode[status];
    }
    
private:
    static std::map<int, std::string> statusCode;
};

#endif // HTTPRESPONSECODES_H
