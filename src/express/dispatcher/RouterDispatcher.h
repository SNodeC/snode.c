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

#ifndef EXPRESS_DISPATCHER_ROUTERDISPATCHER_H
#define EXPRESS_DISPATCHER_ROUTERDISPATCHER_H

#include "express/Dispatcher.h" // IWYU pragma: export

// IWYU pragma: no_include "express/Route.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

// The RouterDispatcher is a complex implementation of the composite pattern (https://en.wikipedia.org/wiki/Composite_pattern)

namespace express::dispatcher {

    class RouterDispatcher : public express::Dispatcher {
    public:
        std::list<express::Route>& getRoutes();

        std::list<std::string> getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint) const;

        RouterDispatcher& setStrictRouting(bool strictRouting);
        bool getStrictRouting() const;

        RouterDispatcher& setCaseInsensitiveRouting(bool caseInsensitiveRouting);
        bool getCaseInsensitiveRouting() const;

        RouterDispatcher& setMergeParams(bool mergeParams);
        bool getMergeParams() const;

    private:
        bool dispatch(express::Controller& controller,
                      const express::MountPoint& mountPoint,
                      bool strictRoutingUnused,
                      bool caseInsensitiveRoutingUnused,
                      bool mergeParamsUnused) override;

        std::list<std::string>
        getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint, bool strictRouting) const override;

        std::list<express::Route> routes;

        bool strictRouting = false;
        bool caseInsensitiveRouting = true;
        bool mergeParams = false;
    };

} // namespace express::dispatcher

#endif // EXPRESS_DISPATCHER_ROUTERDISPATCHER_H
