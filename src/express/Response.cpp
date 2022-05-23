/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "express/Response.h"

#include "core/file/FileReader.h"
#include "web/http/MimeTypes.h"
#include "web/http/StatusCodes.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno> // for errno, EACCES, ENOENT
#include <filesystem>
#include <map>          // for map
#include <system_error> // for error_code

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    Response::Response(web::http::server::RequestContextBase* requestContext)
        : web::http::server::Response(requestContext) {
    }

    void Response::sendFile(const std::string& file, const std::function<void(int err)>& onError) {
        std::string absolutFileName = file;

        if (std::filesystem::exists(absolutFileName)) {
            std::error_code ec;
            absolutFileName = std::filesystem::canonical(absolutFileName);

            if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                core::file::FileReader::connect(absolutFileName, *this, [this, &absolutFileName, onError](int err) -> void {
                    if (err == 0) {
                        headers.insert({{"Content-Type", web::http::MimeTypes::contentType(absolutFileName)},
                                        {"Last-Modified", httputils::file_mod_http_date(absolutFileName)}});
                        headers.insert_or_assign("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName)));
                    } else {
                        onError(err);
                        sendStatus(500);
                    }
                });
            } else {
                errno = EACCES;
                onError(errno);
                sendStatus(500);
            }
        } else {
            errno = ENOENT;
            onError(errno);
            sendStatus(404);
        }
    }

    void Response::download(const std::string& file, const std::function<void(int err)>& onError) {
        std::string name = file;

        if (name[0] == '/') {
            name.erase(0, 1);
        }

        download(file, name, onError);
    }

    void Response::download(const std::string& file, const std::string& name, const std::function<void(int err)>& onError) {
        set({{"Content-Disposition", "attachment; filename=\"" + name + "\""}});
        sendFile(file, onError);
    }

    void Response::redirect(const std::string& name) {
        redirect(302, name);
    }

    void Response::redirect(int status, const std::string& name) {
        this->status(status).set({{"Location", name}});
        end();
    }

    void Response::sendStatus(int status) {
        this->status(status).send(web::http::StatusCode::reason(status));
    }

} // namespace express
