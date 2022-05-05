/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022  Volker Christian <me@vchrist.at>
 * Json Middleware 2020, 2021 Marlene Mayr, Anna Moser, Matteo Prock, Eric Thalhammer
 * Github <MarleneMayr><moseranna><MatteoMatteoMatteo><peregrin-tuk>
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

#include "express/middleware/BasicAuthentication.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    BasicAuthentication::BasicAuthentication(const std::string& userName, const std::string& password, const std::string& realm)
        : userName(userName)
        , password(password)
        , realm(realm) {
        use([realm] MIDDLEWARE(req, res, next) {
            std::string authorization = req.header("Authorization");

            if (authorization.empty()) {
                res.set("WWW-Authenticate", "Basic realm=" + realm);
                res.sendStatus(401);
            } else {
                // TODO: check credential
                next();
            }
        });
    }

    class BasicAuthentication&
    BasicAuthentication::instance(const std::string& userName, const std::string& password, const std::string& realm) {
        // Keep the created authentication middleware alive
        static std::list<std::shared_ptr<class BasicAuthentication>> basicAuthentications;

        BasicAuthentication* basicAuthentication = new BasicAuthentication(userName, password, realm);
        basicAuthentications.push_back(std::shared_ptr<BasicAuthentication>(basicAuthentication));

        return *basicAuthentication;
    }

    class BasicAuthentication& BasicAuthentication(const std::string& userName, const std::string& password, const std::string& realm) {
        return BasicAuthentication::instance(userName, password, realm);
    }

} // namespace express::middleware
