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

#ifndef EXPRESS_MIDDLEWARE_STATICMIDDLEWARE_H
#define EXPRESS_MIDDLEWARE_STATICMIDDLEWARE_H

#include "express/Router.h"
#include "web/http/CookieOptions.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    class StaticMiddleware : public Router {
        StaticMiddleware(const StaticMiddleware&) = delete;
        StaticMiddleware& operator=(const StaticMiddleware&) = delete;

    protected:
        StaticMiddleware(const std::string& root);
        static class StaticMiddleware& instance(const std::string& root);

    public:
        StaticMiddleware& clearStdHeaders();
        StaticMiddleware& setStdHeaders(const std::map<std::string, std::string>& stdHeaders);
        StaticMiddleware& appendStdHeaders(const std::map<std::string, std::string>& stdHeaders);
        StaticMiddleware& appendStdHeaders(const std::string& field, const std::string& value);

        StaticMiddleware&
        appendStdCookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {});

        StaticMiddleware& alwaysClose();

    protected:
        std::string root;
        std::map<std::string, std::string> stdHeaders = {
            {"Cache-Control", "public, max-age=0"}, {"Accept-Ranges", "bytes"}, {"X-Powered-By", "snode.c"}};
        std::map<std::string, web::http::CookieOptions> stdCookies = {};
        bool forceClose = false;

        friend class StaticMiddleware& StaticMiddleware(const std::string& root);
    };

    class StaticMiddleware& StaticMiddleware(const std::string& root);

} // namespace express::middleware

#endif // EXPRESS_MIDDLEWARE_STATICMIDDLEWARE_H
