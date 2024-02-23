/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef EXPRESS_RESPONSE_H
#define EXPRESS_RESPONSE_H

// #include "express/Request.h" // IWYU pragma: export

namespace web::http::server {
    class SocketContext;
    class Response;
} // namespace web::http::server

namespace express {
    class Request;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    class Response {
    public:
        explicit Response(const std::shared_ptr<web::http::server::Response>& response) noexcept;

        explicit Response(Response&) = delete;
        explicit Response(Response&&) noexcept = default;

        Response& operator=(Response&) = delete;
        Response& operator=(Response&&) noexcept = default;

        ~Response();

        web::http::server::SocketContext* getSocketContext() const;

        void json(const nlohmann::json& json);

        void download(const std::string& file, const std::function<void(int err)>& onError);
        void download(const std::string& file, const std::string& fileName, const std::function<void(int err)>& onError);

        void redirect(const std::string& loc);
        void redirect(int state, const std::string& loc);

        void sendStatus(int state);

        Response& attachment(const std::string& fileName = "");
        Response& location(const std::string& loc);
        Response& vary(const std::string& field);

    private:
        std::shared_ptr<web::http::server::Response> responseBase;

    public:
        // Facade to web::http::server::Response (responseBase)
        Response& status(int status);
        Response& append(const std::string& field, const std::string& value);
        Response& set(const std::string& field, const std::string& value, bool overwrite = true);
        Response& set(const std::map<std::string, std::string>& headers, bool overwrite = true);
        Response& type(const std::string& type);
        Response& cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {});
        Response& clearCookie(const std::string& name, const std::map<std::string, std::string>& options = {});

        void send(const char* chunk, std::size_t chunkLen);
        void send(const std::string& chunk);
        void upgrade(const std::shared_ptr<Request>& request, const std::function<void(bool success)>& status);
        void end();
        void sendFile(const std::string& file, const std::function<void(int errnum)>& callback);

        Response& sendHeader();
        Response& sendFragment(const char* chunk, std::size_t chunkLen);
        Response& sendFragment(const std::string& chunk);

        const std::string& header(const std::string& field);
    };

} // namespace express

#endif // EXPRESS_RESPONSE_H
