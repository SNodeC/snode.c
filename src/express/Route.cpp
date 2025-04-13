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

#include "express/Route.h"

#include "express/Controller.h"
#include "express/dispatcher/ApplicationDispatcher.h"
#include "express/dispatcher/MiddlewareDispatcher.h"
#include "express/dispatcher/RouterDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define DEFINE_ROUTE_REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                    \
    Route& Route::METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, Next&)>& lambda)      \
        const {                                                                                                                            \
        return *(dispatcher->nextRoute = std::make_shared<Route>(                                                                          \
                     HTTP_METHOD, mountPoint.relativeMountPath, std::make_shared<dispatcher::MiddlewareDispatcher>(lambda)))               \
                    .get();                                                                                                                \
    }                                                                                                                                      \
    Route& Route::METHOD(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& lambda) const {     \
        return *(dispatcher->nextRoute = std::make_shared<Route>(                                                                          \
                     HTTP_METHOD, mountPoint.relativeMountPath, std::make_shared<dispatcher::ApplicationDispatcher>(lambda)))              \
                    .get();                                                                                                                \
    }

namespace express {

    Route::Route()
        : mountPoint("use", "")
        , dispatcher(std::make_shared<dispatcher::RouterDispatcher>()) {
    }

    Route::Route(const std::string& method, const std::string& relativeMountPath, const std::shared_ptr<Dispatcher>& dispatcher)
        : mountPoint(method, relativeMountPath)
        , dispatcher(dispatcher) {
    }

    bool Route::dispatch(Controller& controller) {
        return dispatch(controller, "");
    }

    bool Route::dispatch(Controller& controller, const std::string& parentMountPath) {
        controller.setCurrentRoute(this);

        const bool originalIsInherit = strictRouting == StrictRouting::INHERIT;

        if (originalIsInherit) {
            strictRouting = controller.getStrictRouting() ? StrictRouting::STRICT : StrictRouting::LAX;
        }

        const bool oldStrictRouting = controller.setStrictRouting(strictRouting == StrictRouting::STRICT);

        bool dispatched = dispatcher->dispatch(controller, parentMountPath, mountPoint);

        controller.setStrictRouting(oldStrictRouting);

        if (originalIsInherit) {
            strictRouting = StrictRouting::INHERIT;
        }

        if (!dispatched) { // TODO: only call if parent route matched
            dispatched = controller.dispatchNext(parentMountPath);
        }

        return dispatched;
    }

    bool Route::dispatchNext(Controller& controller, const std::string& parentMountPath) {
        return dispatcher->dispatchNext(controller, parentMountPath);
    }

    std::list<std::string> Route::getRoute(const std::string& parentMountPath, bool strictRouting) const {
        const bool concreteStrictRouting =
            this->strictRouting == StrictRouting::INHERIT ? strictRouting : this->strictRouting == StrictRouting::STRICT;

        return dispatcher->getRoutes(parentMountPath, mountPoint, concreteStrictRouting);
    }

    Route& Route::setStrictRouting(bool strict) {
        this->strictRouting = strict ? StrictRouting::STRICT : StrictRouting::LAX;

        return *this;
    }

    const Route::StrictRouting& Route::getStrictRouting() const {
        return strictRouting;
    }

    DEFINE_ROUTE_REQUESTMETHOD(use, "use")
    DEFINE_ROUTE_REQUESTMETHOD(all, "all")
    DEFINE_ROUTE_REQUESTMETHOD(get, "GET")
    DEFINE_ROUTE_REQUESTMETHOD(put, "PUT")
    DEFINE_ROUTE_REQUESTMETHOD(post, "POST")
    DEFINE_ROUTE_REQUESTMETHOD(del, "DELETE")
    DEFINE_ROUTE_REQUESTMETHOD(connect, "CONNECT")
    DEFINE_ROUTE_REQUESTMETHOD(options, "OPTIONS")
    DEFINE_ROUTE_REQUESTMETHOD(trace, "TRACE")
    DEFINE_ROUTE_REQUESTMETHOD(patch, "PATCH")
    DEFINE_ROUTE_REQUESTMETHOD(head, "HEAD")

} // namespace express
