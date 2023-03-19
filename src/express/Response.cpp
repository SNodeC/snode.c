/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "web/http/StatusCodes.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <filesystem>
#include <nlohmann/json.hpp>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    Response::Response(web::http::server::RequestContextBase* requestContext)
        : web::http::server::Response(requestContext) {
    }

    Response::~Response() {
    }

    void Response::json(const nlohmann::json& json) {
        set("Content-Type", "application/json");
        send(json.dump());
    }

    void Response::download(const std::string& file, const std::function<void(int err)>& onError) {
        download(file, std::filesystem::path(file).filename(), onError);
    }

    void Response::download(const std::string& file, const std::string& fileName, const std::function<void(int err)>& onError) {
        attachment(fileName);
        sendFile(file, onError);
    }

    void Response::redirect(const std::string& loc) {
        redirect(302, loc);
    }

    void Response::redirect(int state, const std::string& loc) {
        location(loc);
        sendStatus(state);
    }

    void Response::sendStatus(int state) {
        this->status(state).send(web::http::StatusCode::reason(state));
    }

    Response& Response::attachment(const std::string& fileName) {
        set("Content-Disposition", "attachment" + ((!fileName.empty()) ? ("; filename=\"" + fileName + "\"") : ""));
        return *this;
    }

    Response& Response::location(const std::string& loc) {
        set("Location", loc);
        return *this;
    }

    Response& Response::vary(const std::string& field) {
        append("Vary", field);
        return *this;
    }

} // namespace express
