/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef WEB_HTTP_CLIENT_REQUESTCOMMAND_H
#define WEB_HTTP_CLIENT_REQUESTCOMMAND_H

namespace web::http::client {
    class Request;  // IWYU pragma: export
    class Response; // IWYU pragma: export
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional> // IWYU pragma: export
#include <memory>     // IWYU pragma: export
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    class RequestCommand {
    public:
        RequestCommand(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                       const std::function<void(const std::string&)>& onResponseParseError);

        RequestCommand(const RequestCommand&) = delete;
        RequestCommand(RequestCommand&&) noexcept = delete;

        RequestCommand& operator=(const RequestCommand&) = delete;
        RequestCommand& operator=(RequestCommand&&) noexcept = delete;

        virtual ~RequestCommand();

        virtual bool execute(std::shared_ptr<Request> request) = 0;

        bool getError() const;

        const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)> onResponseReceived;
        const std::function<void(const std::string&)> onResponseParseError;

    protected:
        bool error = false;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_REQUESTCOMMAND_H
