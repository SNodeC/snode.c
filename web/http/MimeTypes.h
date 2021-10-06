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

#ifndef WEB_HTTP_MIMETYPES_H
#define WEB_HTTP_MIMETYPES_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef HAS_LIBMAGIC
#include <magic.h>
#endif

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    class MimeTypes {
        MimeTypes(const MimeTypes&) = delete;
        MimeTypes operator=(const MimeTypes&) = delete;

    public:
        ~MimeTypes();

        static std::string contentType(const std::string& file);

    private:
        MimeTypes();

#ifdef HAS_LIBMAGIC
        static magic_t magic;
#endif

        static MimeTypes mimeTypes;

        static std::map<std::string, std::string> mimeType;
    };

} // namespace web::http

#endif // WEB_HTTP_MIMETYPES_H
