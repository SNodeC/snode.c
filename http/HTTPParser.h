#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class HTTPParser {
public:
    HTTPParser() = default;
    virtual ~HTTPParser() = default;

    void parse(const char* buf, size_t count);

protected:
    // Parser state
    enum struct PAS { FIRSTLINE, HEADER, BODY, COMPLETE, ERROR } PAS = PAS::FIRSTLINE;

    virtual void reset();

    virtual enum PAS parseStartLine(std::string& line) = 0;
    virtual enum PAS parseHeader() = 0;
    virtual void parseBodyData(char* body, size_t size) = 0;
    virtual void parsingError(int code, const std::string& reason) = 0;

    std::map<std::string, std::string> header;

    enum struct HTTPCompliance : unsigned short {
        RFC1945 = 0x01 << 0, // HTTP 1.0
        RFC2616 = 0x01 << 1, // HTTP 1.1
        RFC7230 = 0x01 << 2, // Message Syntax and Routing
        RFC7231 = 0x01 << 3, // Semantics and Content
        RFC7232 = 0x01 << 4, // Conditional Requests
        RFC7233 = 0x01 << 5, // Range Requests
        RFC7234 = 0x01 << 6, // Caching
        RFC7235 = 0x01 << 7, // Authentication
        RFC7540 = 0x01 << 8, // HTTP 2.0
        RFC7541 = 0x01 << 9  // Header Compression

    } HTTPCompliance{HTTPCompliance::RFC2616 | HTTPCompliance::RFC7230};

protected:
    bool EOL{false};
    size_t contentLength = 0;

private:
    size_t readStartLine(const char* buf, size_t count);
    size_t readHeaderLine(const char* buf, size_t count);
    void splitHeaderLine(const std::string& line);
    size_t readBodyData(const char* buf, size_t count);

    // Parseing data
    std::string line;
    size_t contentRead = 0;
    char* bodyData = nullptr;

    friend enum HTTPCompliance operator|(const enum HTTPCompliance& c1, const enum HTTPCompliance& c2);
    friend enum HTTPCompliance operator&(const enum HTTPCompliance& c1, const enum HTTPCompliance& c2);
};

#endif // HTTPPARSER_H
