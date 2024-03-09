/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "web/http/decoder/Header.h"

#include "core/socket/stream/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <cctype>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define MAX_LINE_LENGTH 8192
// #define MAX_LINE_LENGTH 1

namespace web::http::decoder {

    Header::Header(core::socket::stream::SocketContext* socketContext, std::set<std::string> fieldsExpected)
        : socketContext(socketContext)
        , fieldsExpected(std::move(fieldsExpected))
        , maxLineLength(MAX_LINE_LENGTH) {
    }

    void Header::setFieldsExpected(std::set<std::string> fieldsExpected) {
        this->fieldsExpected = std::move(fieldsExpected);
    }

    std::size_t Header::read() {
        std::size_t consumed = 0;

        if (completed || errorCode != 0) {
            completed = false;
            mapToFill.clear();
            errorCode = 0;
            errorReason = "";
        }

        std::size_t ret = 0;

        do {
            char ch = '\0';

            ret = socketContext->readFromPeer(&ch, 1);
            consumed += ret;

            if (ret > 0) {
                if (!line.empty() || ch != ' ') {
                    line += ch;

                    if (maxLineLength == 0 || line.size() <= maxLineLength) {
                        lastButOne = last;
                        last = ch;

                        if (lastButOne == '\r' && last == '\n') {
                            line.pop_back(); // Remove \n
                            line.pop_back(); // Remove \r#

                            completed = line.empty();
                            if (!completed) {
                                splitLine(line);

                                if (!fieldsExpected.empty() && mapToFill.size() > fieldsExpected.size()) {
                                    errorCode = 400;
                                    errorReason = "Too many fields";
                                }
                            } else if (!fieldsExpected.empty() && mapToFill.size() < fieldsExpected.size()) {
                                errorCode = 400;
                                errorReason = "Too view fields";

                                completed = false;
                            }
                            line.clear();
                            lastButOne = '\0';
                            last = '\0';
                        }
                    } else {
                        errorCode = 400;
                        errorReason = "Line too long";
                    }
                } else {
                    errorCode = 400;
                    errorReason = "Header Folding";
                }
            }
        } while (ret > 0 && !completed && errorCode == 0);

        return consumed;
    }

    void Header::splitLine(const std::string& line) {
        auto [headerFieldName, value] = httputils::str_split(line, ':');

        if (headerFieldName.empty()) {
            errorCode = 400;
            errorReason = "Header field empty";
        } else if ((std::isblank(headerFieldName.back()) != 0) || (std::isblank(headerFieldName.front()) != 0)) {
            errorCode = 400;
            errorReason = "White space before or after header-field";
        } else if (value.empty()) {
            errorCode = 400;
            errorReason = "Header-value of field \"" + headerFieldName + "\" empty";
        } else {
            httputils::str_trimm(value);

            if (mapToFill.find(headerFieldName) == mapToFill.end()) {
                mapToFill.emplace(headerFieldName, value);
            } else {
                mapToFill[headerFieldName] += "," + value;
            }
        }
    }

    web::http::CiStringMap<std::string>&& Header::getHeader() {
        return std::move(mapToFill);
    }

    bool Header::isComplete() const {
        return completed;
    }

    bool Header::isError() const {
        return errorCode != 0;
    }

    int Header::getErrorCode() const {
        return errorCode;
    }

    std::string Header::getErrorReason() {
        return errorReason;
    }

} // namespace web::http::decoder
