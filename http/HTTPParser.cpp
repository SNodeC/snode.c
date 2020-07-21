#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cctype>
#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPParser.h"
#include "httputils.h"


void HTTPParser::reset() {
    PAS = PAS::FIRSTLINE;
    contentLength = 0;
    if (bodyData != nullptr) {
        delete[] bodyData;
        bodyData = nullptr;
    }
}


void HTTPParser::parse(const char* buf, size_t count) {
    size_t processed = 0;

    while (processed < count && PAS != PAS::ERROR) {
        switch (PAS) {
        case PAS::FIRSTLINE:
            processed += readStartLine(buf + processed, count - processed);
            break;
        case PAS::HEADER:
            processed += readHeaderLine(buf + processed, count - processed);
            break;
        case PAS::BODY:
            processed += readBodyData(buf + processed, count - processed);
            break;
        case PAS::ERROR:
            break;
        };
    }
}


size_t HTTPParser::readStartLine(const char* buf, size_t count) {
    size_t consumed = 0;

    while (consumed < count && PAS == PAS::FIRSTLINE) {
        char ch = buf[consumed];

        if (ch == '\r' || ch == '\n') {
            consumed++;
            if (ch == '\n') {
                parseStartLine(line);
                line.clear();
                PAS = PAS::HEADER;
            }
        } else {
            line += ch;
            consumed++;
        }
    }

    return consumed;
}


size_t HTTPParser::readHeaderLine(const char* buf, size_t count) {
    size_t consumed = 0;
    while (consumed < count && PAS == PAS::HEADER) {
        char ch = buf[consumed];

        if (ch == '\r' || ch == '\n') {
            consumed++;
            if (ch == '\n') {
                if (EOL) {
                    splitHeaderLine(line);
                    line.clear();
                    if (contentLength == 0) {
                        parsingFinished();
                    } else {
                        PAS = PAS::BODY;
                    }
                    EOL = false;
                } else {
                    if (line.empty()) {
                        parsingFinished();
                    } else {
                        EOL = true;
                    }
                }
            }
        } else if (EOL) {
            if (std::isblank(ch)) {
                if ((HTTPCompliance & HTTPCompliance::RFC7230) == HTTPCompliance::RFC7230) {
                    PAS = PAS::ERROR;
                    parsingError(400, "Header Folding");
                } else {
                    line += ch;
                    consumed++;
                }
            } else {
                splitHeaderLine(line);
                line.clear();
            }
            EOL = false;
        } else {
            line += ch;
            consumed++;
        }
    }

    return consumed;
}


void HTTPParser::splitHeaderLine(const std::string& line) {
    if (!line.empty()) {
        std::string field;
        std::string value;
        std::tie(field, value) = httputils::str_split(line, ':');
        
        if (field.empty()) {
            parsingError(400, "Header-field empty");
            PAS = PAS::ERROR;
        } else if (std::isblank(field.back()) || std::isblank(field.front())) {
            parsingError(400, "White space before or after header-field");
            PAS = PAS::ERROR;
        } else if (value.empty()) {
            parsingError(400, "Header-value of field \"" + field + "\" empty");
            PAS = PAS::ERROR;
        } else {
            httputils::str_trimm(value);
            httputils::to_lower(field);
            
            parseHeaderLine(field, value);
        }
    } else {
        parsingError(400, "Header-line empty");
        PAS = PAS::ERROR;
    }
}


size_t HTTPParser::readBodyData(const char* buf, size_t count) {
    if (contentRead == 0) {
        bodyData = new char[contentLength];
    }

    if (contentRead + count <= contentLength) {
        memcpy(bodyData + contentRead, buf, count);

        contentRead += count;
        if (contentRead == contentLength) {
            parseBodyData(bodyData, contentLength);
            parsingFinished();

            delete[] bodyData;
            bodyData = nullptr;
            contentRead = 0;
        }
    } else {
        PAS = PAS::ERROR;
        parsingError(400, "Content to long");

        if (bodyData != nullptr) {
            delete[] bodyData;
            bodyData = nullptr;
        }
    }

    return count;
}


enum HTTPParser::HTTPCompliance operator|(const enum HTTPParser::HTTPCompliance& c1, const enum HTTPParser::HTTPCompliance& c2) {
    return static_cast<enum HTTPParser::HTTPCompliance>(static_cast<unsigned short>(c1) | static_cast<unsigned short>(c2));
}

enum HTTPParser::HTTPCompliance operator&(const enum HTTPParser::HTTPCompliance& c1, const enum HTTPParser::HTTPCompliance& c2) {
    return static_cast<enum HTTPParser::HTTPCompliance>(static_cast<unsigned short>(c1) & static_cast<unsigned short>(c2));
}
