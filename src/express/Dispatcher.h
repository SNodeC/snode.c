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

#ifndef EXPRESS_DISPATCHER_H
#define EXPRESS_DISPATCHER_H

namespace express {
    class Route;
    class Controller;
    struct MountPoint;
} // namespace express

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::detail {} // namespace express::detail

namespace express {

    class Dispatcher {
    public:
        Dispatcher(const Dispatcher&) = delete;

        Dispatcher& operator=(const Dispatcher&) = delete;

        Dispatcher() = default;

        virtual ~Dispatcher();

        virtual bool dispatch(Controller& controller, const std::string& parentMountPath, const MountPoint& mountPoint) = 0;
        bool dispatchNext(Controller& controller, const std::string& parentMountPath);

        virtual std::list<std::string>
        getRoutes(const std::string& parentMountPath, const MountPoint& mountPoint, bool strictRouting) const = 0;

    protected:
        std::shared_ptr<Route> nextRoute = nullptr;

    private:
        friend class Route;
    };

} // namespace express

#endif // EXPRESS_DISPATCHER_H
