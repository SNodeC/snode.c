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

#ifndef EXPRESS_DISPATCHER_CONTROLLER_H
#define EXPRESS_DISPATCHER_CONTROLLER_H

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

namespace express {
    class Request;
    class Response;
    class RootRoute;
    class Route;
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class Controller {
    public:
        Controller(const std::shared_ptr<web::http::server::Request>& request,
                   const std::shared_ptr<web::http::server::Response>& response);
        Controller(const Controller& controller);

        Controller& operator=(const Controller& controller) noexcept;

        void setRootRoute(RootRoute* rootRoute);
        void setCurrentRoute(Route* currentRoute);

        const std::shared_ptr<Request>& getRequest();
        const std::shared_ptr<Response>& getResponse();

        int getFlags() const;

        void next(const std::string& how) const;
        bool nextRouterCalled();
        bool dispatchNext(const std::string& parentMountPath);
        /*
                bool setStrictRouting(bool strictRouting);
                bool getStrictRouting() const;

                bool setCaseInsensitiveRouting(bool caseInsensitiveRouting);
                bool getCaseInsensitiveRouting() const;

                bool setMergeParams(bool mergeParams);
                bool getMergeParams() const;
        */
        enum Flags { NONE = 0, NEXT = 1 << 0, NEXT_ROUTE = 1 << 1, NEXT_ROUTER = 1 << 2 };

    private:
        std::shared_ptr<Request> request;
        std::shared_ptr<Response> response;

        RootRoute* rootRoute = nullptr;

        mutable Route* lastRoute = nullptr;
        Route* currentRoute = nullptr;

        bool strictRouting = false;
        bool caseInsensitiveRouting = true;
        bool mergeParams = false;

        unsigned long lastTick = 0;

        mutable int flags = NONE;
    };

} // namespace express

#endif // EXPRESS_DISPATCHER_CONTROLLER_H
