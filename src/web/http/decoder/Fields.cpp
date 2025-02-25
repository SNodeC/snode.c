/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "web/http/decoder/Fields.h"

#include "core/socket/stream/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <cctype>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define MAX_LINE_LENGTH 8192
// #define MAX_LINE_LENGTH 1

namespace web::http::decoder {

    Fields::Fields(core::socket::stream::SocketContext* socketContext, std::set<std::string> fieldsExpected)
        : socketContext(socketContext)
        , fieldsExpected(std::move(fieldsExpected))
        , maxLineLength(MAX_LINE_LENGTH) {
    }

    void Fields::setFieldsExpected(std::set<std::string> fieldsExpected) {
        this->fieldsExpected = std::move(fieldsExpected);
    }

    std::size_t Fields::read() {
        std::size_t consumed = 0;

        if (completed || errorCode != 0) {
            completed = false;
            fields.clear();
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
                            line.pop_back(); // Remove \r

                            completed = line.empty();
                            if (!completed) {
                                splitLine(line);

                                if (!fieldsExpected.empty() && fields.size() > fieldsExpected.size()) {
                                    errorCode = 400;
                                    errorReason = "Too many fields";
                                }
                            } else if (!fieldsExpected.empty() && fields.size() < fieldsExpected.size()) {
                                errorCode = 400;
                                errorReason = "Too view fields";

                                completed = false;
                            }
                            line.clear();
                            lastButOne = '\0';
                            last = '\0';
                        }
                    } else {
                        errorCode = 431;
                        errorReason = "Line too long: " + line;
                    }
                } else {
                    errorCode = 400;
                    errorReason = "Header Folding";
                }
            }
        } while (ret > 0 && !completed && errorCode == 0);

        return consumed;
    }

    void Fields::splitLine(const std::string& line) {
        auto [headerFieldName, value] = httputils::str_split(line, ':');

        if (headerFieldName.empty()) {
            errorCode = 400;
            errorReason = "Header field empty";
        } else if ((std::isblank(headerFieldName.back()) != 0) || (std::isblank(headerFieldName.front()) != 0)) {
            errorCode = 400;
            errorReason = "White space before or after field";
        } else if (value.empty()) {
            errorCode = 400;
            errorReason = "Value of field \"" + headerFieldName + "\" empty";
        } else {
            if (fieldsExpected.empty() || fieldsExpected.contains(headerFieldName)) {
                httputils::str_trimm(value);

                if (!fields.contains(headerFieldName)) {
                    fields.emplace(headerFieldName, value);
                } else {
                    fields[headerFieldName] += "," + value;
                }
            } else if (!fieldsExpected.empty() && !fieldsExpected.contains(headerFieldName)) {
                errorCode = 400;
                errorReason = "Field '" + headerFieldName + "' not in expected fields";
            }
        }
    }

    web::http::CiStringMap<std::string>&& Fields::getHeader() {
        return std::move(fields);
    }

    bool Fields::isComplete() const {
        return completed;
    }

    bool Fields::isError() const {
        return errorCode != 0;
    }

    int Fields::getErrorCode() const {
        return errorCode;
    }

    std::string Fields::getErrorReason() {
        return errorReason;
    }

} // namespace web::http::decoder
