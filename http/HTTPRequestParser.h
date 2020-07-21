#ifndef HTTPREQUESTPARSER_H
#define HTTPREQUESTPARSER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <set>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPParser.h"


class HTTPRequestParser : public HTTPParser {
public:
    HTTPRequestParser(const std::function<void(std::string&, std::string&, std::string&)>& onRequest,
                      const std::function<void(const std::string&, const std::string&)>& onHeader,
                      const std::function<void(const std::string&, const std::string&)>& onCookie,
                      const std::function<void(char* body, size_t bodyLength)>& onBody, const std::function<void(void)>& onParsed,
                      const std::function<void(int status, const std::string& reason)>& onError);

    HTTPRequestParser(std::function<void(std::string&, std::string&, std::string&)>&& onRequest,
                      std::function<void(const std::string&, const std::string&)>&& onHeader,
                      std::function<void(const std::string&, const std::string&)>&& onCookie,
                      std::function<void(char* body, size_t bodyLength)>&& onBody, std::function<void(void)>&& onParsed,
                      std::function<void(int status, const std::string& reason)>&& onError);

    void reset() override;

protected:
    // Check if request method is supported
    virtual bool methodSupported(const std::string& method) {
        return supportedMethods.contains(method);
    }

    // Parsers and Validators
    void parseStartLine(std::string& line) override;
    void parseHeaderLine(const std::string& field, const std::string& value) override;
    void parseBodyData(char* body, size_t size) override;

    // Exits
    void parsingFinished() override;
    void parsingError(int code, const std::string& reason) override;

    // Supported http-methods
    std::set<std::string> supportedMethods{"GET", "PUT", "POST", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH", "HEAD"};

    // Parsing results
    std::string method;
    std::string originalUrl;
    std::string httpVersion;
    int httpMajor = 0;
    int httpMinor = 0;

    std::function<void(std::string&, std::string&, std::string&)> onRequest;
    std::function<void(const std::pair<std::string, std::string>&)> onQuery;
    std::function<void(const std::string& field, const std::string& value)> onHeader;
    std::function<void(const std::string& name, const std::string& value)> onCookie;
    std::function<void(char* body, size_t bodyLength)> onBody;
    std::function<void(void)> onParsed;
    std::function<void(int status, const std::string& reason)> onError;
};

#endif // HTTPREQUESTPARSER_H
