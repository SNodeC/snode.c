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

namespace web::http::client {
    class RequestCommand;
    class Response;
    class SocketContext;
    namespace commands {
        class SendFileCommand;
        class SendFragmentCommand;
        class SendHeaderCommand;
        class UpgradeCommand;
        class EndCommand;
    } // namespace commands
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // IWYU pragma: export
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    class Request : public core::pipe::Sink {
    public:
        explicit Request(web::http::client::SocketContext* clientContext);

        explicit Request(Request&) = delete;
        explicit Request(Request&&) noexcept = default;

        Request& operator=(Request&) = delete;
        Request& operator=(Request&&) noexcept = default;

        ~Request() override;

        virtual void init(const std::string& host);
        void stopResponse();

    private:
        void execute();

    public:
        Request& host(const std::string& host);
        Request& append(const std::string& field, const std::string& value);
        Request& set(const std::string& field, const std::string& value, bool overwrite = true);
        Request& set(const std::map<std::string, std::string>& headers, bool overwrite = true);
        Request& type(const std::string& type);
        Request& cookie(const std::string& name, const std::string& value);
        Request& cookie(const std::map<std::string, std::string>& cookies);

        static void responseParseError(const std::shared_ptr<Request>& request, const std::string& message);

        void
        send(const char* junk,
             std::size_t junkLen,
             const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
             const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError = responseParseError);
        void
        send(const std::string& junk,
             const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
             const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError = responseParseError);
        void
        upgrade(const std::string& url,
                const std::string& protocols,
                const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError = responseParseError);
        void upgrade(const std::shared_ptr<Response>& response, const std::function<void(bool)>& status);
        void
        sendFile(const std::string& file,
                 const std::function<void(int errnum)>& onStatus,
                 const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                 const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError = responseParseError);
        void end(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                 const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError = responseParseError);

        Request& sendHeader();
        Request& sendFragment(const char* junk, std::size_t junkLen);
        Request& sendFragment(const std::string& data);

    private:
        void deliverResponse(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response);
        void deliverResponseParseError(const std::shared_ptr<Request>& request, const std::string& message);

        void dispatchSendHeader();
        void dispatchSendFragment(const char* junk, std::size_t junkLen);
        void dispatchSendFile(const std::string& file, const std::function<void(int errnum)>& onStatus);
        void dispatchUpgrade(const std::string& url, const std::string& protocols);
        void dispatchEnd();

        void dispatchCompleted() const;

        friend class commands::SendFileCommand;
        friend class commands::SendFragmentCommand;
        friend class commands::SendHeaderCommand;
        friend class commands::UpgradeCommand;
        friend class commands::EndCommand;

        void onSourceConnect(core::pipe::Source* source) override;
        void onSourceData(const char* junk, std::size_t junkLen) override;
        void onSourceEof() override;
        void onSourceError(int errnum) override;

    public:
        const std::string& header(const std::string& field);

        web::http::client::SocketContext* getSocketContext() const;

    private:
        std::list<RequestCommand*> requestCommands;

        std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)> onResponseReceived;
        std::function<void(const std::shared_ptr<Request>& request, const std::string& message)> onResponseParseError;

    public:
        std::string method = "GET";
        std::string url;
        int httpMajor = 1;
        int httpMinor = 1;

        //    protected:
        std::map<std::string, std::string> queries;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> cookies;

        //    private:
        std::size_t contentSent = 0;
        std::size_t contentLength = 0;

        web::http::client::SocketContext* socketContext;

        ConnectionState connectionState = ConnectionState::Default;

        friend class SocketContext;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_REQUEST_H
