/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "express/Response.h"

#include "express/Request.h"
#include "web/http/StatusCodes.h"
#include "web/http/server/Response.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <filesystem>
#include <nlohmann/json.hpp>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    Response::Response(const std::shared_ptr<web::http::server::Response>& response) noexcept
        : responseBase(response) {
    }

    Response::~Response() {
    }

    web::http::server::SocketContext* Response::getSocketContext() const {
        return responseBase->getSocketContext();
    }

    void Response::json(const nlohmann::json& json) {
        set("Content-Type", "application/json");
        send(json.dump());
    }

    void Response::download(const std::string& file, const std::function<void(int)>& onError) {
        download(file, std::filesystem::path(file).filename(), onError);
    }

    void Response::download(const std::string& file, const std::string& fileName, const std::function<void(int)>& onError) {
        attachment(fileName);
        sendFile(file, onError);
    }

    void Response::redirect(const std::string& loc, const std::string& html) {
        redirect(302, loc, html);
    }

    void Response::redirect(int state, const std::string& loc, const std::string& html) {
        location(loc);
        sendStatus(state, html);
    }

    void Response::sendStatus(int state, const std::string& html) {
        this->status(state).send(!html.empty() ? html : web::http::StatusCode::reason(state) + "\r\n");
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

    Response& Response::status(int status) {
        responseBase->status(status);

        return *this;
    }

    Response& Response::append(const std::string& field, const std::string& value) {
        responseBase->append(field, value);

        return *this;
    }

    Response& Response::set(const std::string& field, const std::string& value, bool overwrite) {
        responseBase->set(field, value, overwrite);

        return *this;
    }

    Response& Response::set(const std::map<std::string, std::string>& headers, bool overwrite) {
        responseBase->set(headers, overwrite);

        return *this;
    }

    Response& Response::type(const std::string& type) {
        responseBase->type(type);

        return *this;
    }

    Response& Response::cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options) {
        responseBase->cookie(name, value, options);

        return *this;
    }

    Response& Response::clearCookie(const std::string& name, const std::map<std::string, std::string>& options) {
        responseBase->clearCookie(name, options);

        return *this;
    }

    Response& Response::setTrailer(const std::string& field, const std::string& value, bool overwrite) {
        responseBase->setTrailer(field, value, overwrite);

        return *this;
    }

    void Response::send(const char* chunk, std::size_t chunkLen) {
        responseBase->send(chunk, chunkLen);
    }

    void Response::send(const std::string& chunk) {
        responseBase->send(chunk);
    }

    void Response::upgrade(const std::shared_ptr<Request>& request, const std::function<void(const std::string)>& status) {
        responseBase->upgrade(request->requestBase, status);
    }

    void Response::end() {
        responseBase->end();
    }

    void Response::sendFile(const std::string& file, const std::function<void(int)>& callback) {
        responseBase->sendFile(file, callback);
    }

    Response& Response::sendHeader() {
        responseBase->sendHeader();

        return *this;
    }

    Response& Response::sendFragment(const char* chunk, std::size_t chunkLen) {
        responseBase->sendFragment(chunk, chunkLen);

        return *this;
    }

    Response& Response::sendFragment(const std::string& chunk) {
        responseBase->sendFragment(chunk);

        return *this;
    }

    const std::string& Response::header(const std::string& field) {
        return responseBase->header(field);
    }

} // namespace express
