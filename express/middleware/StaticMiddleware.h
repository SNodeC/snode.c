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

#ifndef STATICMIDDLEWARE_H
#define STATICMIDDLEWARE_H

#include "Router.h"

class StaticMiddleware : public Router {
protected:
    StaticMiddleware(const std::string& root);
    static const StaticMiddleware& instance(const std::string& root);

public:
    StaticMiddleware(const StaticMiddleware&) = delete;
    StaticMiddleware& operator=(const StaticMiddleware&) = delete;

protected:
    std::string root;

    friend const class StaticMiddleware& StaticMiddleware(const std::string& root);
};

const class StaticMiddleware& StaticMiddleware(const std::string& root);

#endif // STATICMIDDLEWARE_H
