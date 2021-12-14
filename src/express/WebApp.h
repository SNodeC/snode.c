/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef EXPRESS_WEBAPP_H
#define EXPRESS_WEBAPP_H

#include "core/TickStatus.h"
#include "express/Router.h" // IWYU pragma: export
#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <climits>
#include <sys/time.h> // IWYU pragma: keep

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class WebApp : public Router {
    protected:
        explicit WebApp(const Router& router);

    public:
        static void init(int argc, char* argv[]);
        static int start(const utils::Timeval& timeOut = {LONG_MAX, 0});
        static core::TickStatus tick(const utils::Timeval& timeOut = 0);
        static void free();

    private:
        static bool initialized;
    };

} // namespace express

#endif // EXPRESS_WEBAPP_H
