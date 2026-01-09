/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "web/http/StatusCodes.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    // clang-format off

    std::map<int, std::string> StatusCode::statusCode = {{  0, "Connection loss"},
                                                         {100, "Continue"},
                                                         {101, "Switching Protocols"},
                                                         {102, "Processing"},
                                                         {103, "Early Hints"},
                                                         {200, "OK"},
                                                         {201, "Created"},
                                                         {202, "Accepted"},
                                                         {203, "Non-Authoritative Information"},
                                                         {204, "No Content"},
                                                         {205, "Reset Content"},
                                                         {206, "Partial Content"},
                                                         {207, "Multi-Status"},
                                                         {208, "Already Reported"},
                                                         {226, "IM Used"},
                                                         {300, "Multiple Choices"},
                                                         {301, "Moved Permanently"},
                                                         {302, "Found"},
                                                         {303, "See Other"},
                                                         {304, "Not Modified"},
                                                         {305, "Use Proxy"},
                                                         {307, "Temporary Redirect"},
                                                         {308, "Permanent Redirect"},
                                                         {400, "Bad Request"},
                                                         {401, "Unauthorized"},
                                                         {402, "Payment Required"},
                                                         {403, "Forbidden"},
                                                         {404, "Not Found"},
                                                         {405, "Method Not Allowed"},
                                                         {406, "Not Acceptable"},
                                                         {407, "Proxy Authentication Required"},
                                                         {408, "Request Timeout"},
                                                         {409, "Conflict"},
                                                         {410, "Gone"},
                                                         {411, "Length Required"},
                                                         {412, "Precondition Failed"},
                                                         {413, "Payload Too Large"},
                                                         {414, "URI Too Long"},
                                                         {415, "Unsupported Media Type"},
                                                         {416, "Range Not Satisfiable"},
                                                         {417, "Expectation Failed"},
                                                         {421, "Misdirected Request"},
                                                         {422, "Unprocessable Entity"},
                                                         {423, "Locked"},
                                                         {424, "Failed Dependency"},
                                                         {425, "Too Early"},
                                                         {426, "Upgrade Required"},
                                                         {427, "Unassigned"},
                                                         {428, "Precondition Required"},
                                                         {429, "Too Many Requests"},
                                                         {430, "Unassigned"},
                                                         {431, "Request Header Fields Too Large"},
                                                         {451, "Unavailable For Legal Reasons"},
                                                         {500, "Internal Server Error"},
                                                         {501, "Not Implemented"},
                                                         {502, "Bad Gateway"},
                                                         {503, "Service Unavailable"},
                                                         {504, "Gateway Timeout"},
                                                         {505, "HTTP Version Not Supported"},
                                                         {506, "Variant Also Negotiates"},
                                                         {507, "Insufficient Storage"},
                                                         {508, "Loop Detected"},
                                                         {510, "Not Extended"},
                                                         {511, "Network Authentication Required"}};

    // clang-format on

    std::string StatusCode::reason(int status) {
        std::string reasonPhrase = "unknown status code";

        if (StatusCode::contains(status)) {
            reasonPhrase = statusCode[status];
        }

        return reasonPhrase;
    }

    bool StatusCode::contains(int status) {
        return statusCode.contains(status);
    }
} // namespace web::http
