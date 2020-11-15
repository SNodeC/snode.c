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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <filesystem>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "MimeTypes.h"
#include "Response.h"
#include "ServerContext.h"
#include "StatusCodes.h"
#include "file/FileReader.h"
#include "http_utils.h"

namespace express {

    Response::Response(http::ServerContextBase* serverContext)
        : http::Response(serverContext) {
    }

    void Response::sendFile(const std::string& file, const std::function<void(int err)>& onError) {
        std::string absolutFileName = file;

        if (std::filesystem::exists(absolutFileName)) {
            std::error_code ec;
            absolutFileName = std::filesystem::canonical(absolutFileName);

            if (std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
                headers.insert({{"Content-Type", MimeTypes::contentType(absolutFileName)},
                                {"Last-Modified", httputils::file_mod_http_date(absolutFileName)}});
                headers.insert_or_assign("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName)));

                fileReader = FileReader::read(
                    absolutFileName,
                    [this](char* data, int length) -> void {
                        enqueue(data, length);
                    },
                    [this, onError](int err) -> void {
                        onError(err);
                        serverContext->terminateConnection();
                        fileReader = nullptr;
                    });
            } else {
                responseStatus = 403;
                errno = EACCES;
                onError(errno);
                end();
            }
        } else {
            responseStatus = 404;
            errno = ENOENT;
            onError(errno);
            end();
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
        this->status(status).send(StatusCode::reason(status));
    }

    void Response::reset() {
        http::Response::reset();
        if (fileReader != nullptr) {
            fileReader->ReadEventReceiver::disable();
            fileReader->ReadEventReceiver::suspend();
            fileReader = nullptr;
        }
    }

} // namespace express
