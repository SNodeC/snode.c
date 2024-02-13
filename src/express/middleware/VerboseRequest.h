/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef EXPRESS_MIDDLEWARE_VERBOSEREQUEST_H
#define EXPRESS_MIDDLEWARE_VERBOSEREQUEST_H

#include "express/Router.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace express::middleware {

    class VerboseRequest : public express::Router {
    public:
        enum Details {
            W_NONE = 0x00,
            W_REQUEST = 0x01,
            W_QUERIES = 0x02,
            W_HEADERS = 0x05,
            W_COOKIES = 0x08,
            W_CONTENT = 0x10,
            W_ALL = 0x1F
        };

        VerboseRequest(const VerboseRequest&) = delete;
        VerboseRequest& operator=(const VerboseRequest&) = delete;

    protected:
        explicit VerboseRequest(Details detauls);

    private:
        friend class VerboseRequest& VerboseRequest(VerboseRequest::Details details);
    };

    class VerboseRequest& VerboseRequest(VerboseRequest::Details details = VerboseRequest::W_ALL);

} // namespace express::middleware

#endif // EXPRESS_MIDDLEWARE_VERBOSEREQUEST_H
