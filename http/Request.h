/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef REQUEST_H
#define REQUEST_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AttributeInjector.h"

namespace http {

    class ServerContextBase;

    class Request : public utils::MultibleAttributeInjector {
    protected:
        explicit Request() = default;
        virtual ~Request() = default;

    public:
        const std::string& header(const std::string& key, int i = 0) const;
        const std::string& cookie(const std::string& key) const;
        const std::string& query(const std::string& key) const;
        int bodyLength() const;

        // Properties
        std::string method;
        std::string url;
        std::string httpVersion;
        char* body = nullptr;
        int contentLength = 0;

        bool keepAlive = false;

    protected:
        virtual void reset();

        const std::map<std::string, std::string>* queries = nullptr;
        const std::map<std::string, std::string>* headers = nullptr;
        const std::map<std::string, std::string>* cookies = nullptr;

        std::string nullstr = "";

        template <typename Request, typename Response>
        friend class ServerContext;
    };

} // namespace http

#endif // REQUEST_H
