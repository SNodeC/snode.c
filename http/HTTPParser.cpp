/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cctype> // for isblank
#include <cstring>
#include <easylogging++.h>
#include <tuple> // for tie, tuple

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPParser.h"
#include "httputils.h"

namespace http {

    // HTTP/x.x
    std::regex HTTPParser::httpVersionRegex("^HTTP/([[:digit:]])\\.([[:digit:]])$");

    void HTTPParser::reset() {
        PAS = PAS::FIRSTLINE;
        headers.clear();
        contentLength = 0;
        if (content != nullptr) {
            delete[] content;
            content = nullptr;
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
                    processed += readContent(buf + processed, count - processed);
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
                    PAS = parseStartLine(line);
                    line.clear();
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

                        PAS = parseHeader();

                        EOL = false;
                    } else {
                        if (line.empty()) {
                            PAS = parseHeader();
                        } else {
                            EOL = true;
                        }
                    }
                }
            } else if (EOL) {
                if (std::isblank(ch)) {
                    if ((HTTPCompliance & HTTPCompliance::RFC7230) == HTTPCompliance::RFC7230) {
                        PAS = parsingError(400, "Header Folding");
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
                PAS = parsingError(400, "Header-field empty");
            } else if (std::isblank(field.back()) || std::isblank(field.front())) {
                PAS = parsingError(400, "White space before or after header-field");
            } else if (value.empty()) {
                PAS = parsingError(400, "Header-value of field \"" + field + "\" empty");
            } else {
                httputils::to_lower(field);
                httputils::str_trimm(value);
                httputils::to_lower(value);

                if (headers.find(field) == headers.end()) {
                    VLOG(1) << "++ Header (insert): " << field << " = " << value;
                    headers.insert({field, value});
                } else {
                    VLOG(1) << "++ Header (append): " << field << " = " << value;
                    headers[field] += "," + value;
                }
            }
        } else {
            PAS = parsingError(400, "Header-line empty");
        }
    }

    size_t HTTPParser::readContent(const char* buf, size_t count) {
        if (contentRead == 0) {
            content = new char[contentLength];
        }

        if (contentRead + count <= contentLength) {
            memcpy(content + contentRead, buf, count); // NOLINT(clang-analyzer-core.NonNullParamChecker)

            contentRead += count;
            if (contentRead == contentLength) {
                PAS = parseContent(content, contentLength);

                delete[] content;
                content = nullptr;
                contentRead = 0;
            }
        } else {
            PAS = parsingError(400, "Content to long");

            if (content != nullptr) {
                delete[] content;
                content = nullptr;
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

} // namespace http
