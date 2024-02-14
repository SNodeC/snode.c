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

#ifndef WEB_HTTP_CLIENT_REQUEST_H
#define WEB_HTTP_CLIENT_REQUEST_H

#include "core/pipe/Sink.h"
#include "web/http/ConnectionState.h"

namespace core::pipe {
    class Source;
}

namespace web::http {
    class SocketContext;
    namespace client {
        class Response;
    }
} // namespace web::http

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // IWYU pragma: export
#include <functional>
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    class Request : public core::pipe::Sink {
    public:
        explicit Request(web::http::SocketContext* clientContext);

        explicit Request(Request&) = delete;
        explicit Request(Request&&) noexcept = default;

        Request& operator=(Request&) = delete;
        Request& operator=(Request&&) noexcept = default;

        ~Request() override;

    private:
        virtual void reInit();

    public:
        Request& host(const std::string& host);
        Request& append(const std::string& field, const std::string& value);
        Request& set(const std::string& field, const std::string& value, bool overwrite = true);
        Request& set(const std::map<std::string, std::string>& headers, bool overwrite = true);
        Request& type(const std::string& type);
        Request& cookie(const std::string& name, const std::string& value);
        Request& cookie(const std::map<std::string, std::string>& cookies);

        void start();
        void send(const char* junk, std::size_t junkLen);
        void send(const std::string& junk);
        void upgrade(const std::string& url, const std::string& protocols);
        void upgrade(std::shared_ptr<Response>& response, const std::function<void(bool success)>& status);

        void sendFile(const std::string& file, const std::function<void(int errnum)>& callback);
        void stopResponse();

        Request& sendHeader();
        Request& sendFragment(const char* junk, std::size_t junkLen);
        Request& sendFragment(const std::string& data);

    private:
        void sendCompleted();

        void onSourceConnect(core::pipe::Source* source) override;
        void onSourceData(const char* junk, std::size_t junkLen) override;
        void onSourceEof() override;
        void onSourceError(int errnum) override;

    public:
        const std::string& header(const std::string& field);

        web::http::SocketContext* getSocketContext() const;

    private:
    public:
        std::string method = "GET";
        std::string url;
        int httpMajor = 1;
        int httpMinor = 1;

    protected:
        std::map<std::string, std::string> queries;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> cookies;

    private:
        std::size_t contentSent = 0;
        std::size_t contentLength = 0;

        web::http::SocketContext* socketContext;

        ConnectionState connectionState = ConnectionState::Default;

        template <typename Request, typename Response>
        friend class SocketContext;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_REQUEST_H
