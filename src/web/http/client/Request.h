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

#ifndef WEB_HTTP_CLIENT_REQUEST_H
#define WEB_HTTP_CLIENT_REQUEST_H

#include "core/pipe/Sink.h"
#include "web/http/ConnectionState.h"
#include "web/http/TransferEncoding.h"

namespace web::http::client {
    class MasterRequest;
    class RequestCommand;
    class Response;
    class SocketContext;
    namespace commands {
        class SendFileCommand;
        class SendFragmentCommand;
        class SendHeaderCommand;
        class UpgradeCommand;
        class EndCommand;
        class SseCommand;
    } // namespace commands
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {} // namespace web::http::client

// IW YU pragma: no_include "web/http/client/RequestCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/CiStringMap.h" // IWYU pragma: export

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    class Request {
    public:
        Request(SocketContext* socketContext, const std::string& host);

        explicit Request(Request&) = delete;
        explicit Request(Request&& request) noexcept;

        Request& operator=(Request&) = delete;
        Request& operator=(Request&&) noexcept = delete;

        void setMasterRequest(const std::shared_ptr<MasterRequest>& masterRequest);
        std::shared_ptr<MasterRequest> getMasterRequest() const;

        SocketContext* getSocketContext() const;

        Request& host(const std::string& hostFieldValue);
        Request& append(const std::string& field, const std::string& value);
        Request& set(const std::string& field, const std::string& value, bool overwrite = true);
        Request& set(const std::map<std::string, std::string>& headers, bool overwrite = true);
        Request& type(const std::string& type);
        Request& cookie(const std::string& name, const std::string& value);
        Request& cookie(const std::map<std::string, std::string>& cookies);
        Request& query(const std::string& key, const std::string& value);
        Request& setTrailer(const std::string& field, const std::string& value, bool overwrite = true);

        std::string header(const std::string& field) const;
        const CiStringMap<std::string>& getQueries() const;
        const CiStringMap<std::string>& getHeaders() const;
        const CiStringMap<std::string>& getTrailer() const;
        const CiStringMap<std::string>& getCookies() const;

        std::string hostFieldValue;
        std::string method = "GET";
        std::string url = "/";
        int httpMajor = 1;
        int httpMinor = 1;

        std::size_t count{0};

    protected:
        void upgrade(const std::shared_ptr<Response>& response, const std::function<void(const std::string&)>& status);

        CiStringMap<std::string> queries;
        CiStringMap<std::string> headers;
        CiStringMap<std::string> cookies;
        CiStringMap<std::string> trailer;

        std::size_t contentLength = 0;

        std::weak_ptr<MasterRequest> masterRequest;
        web::http::client::SocketContext* socketContext;

        TransferEncoding transferEncoding = TransferEncoding::HTTP10;
        ConnectionState connectionState = ConnectionState::Default;

        friend class MasterRequest;
    };

    class MasterRequest
        : public web::http::client::Request
        , public core::pipe::Sink {
    public:
        MasterRequest(SocketContext* socketContext, const std::string& host);

        explicit MasterRequest(MasterRequest&) = delete;
        explicit MasterRequest(MasterRequest&&) noexcept;

        MasterRequest& operator=(MasterRequest&) = delete;
        MasterRequest& operator=(MasterRequest&&) noexcept = delete;

        ~MasterRequest() override;

        virtual void init();

        bool send(const char* chunk,
                  std::size_t chunkLen,
                  const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                  const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError);
        bool send(const std::string& chunk,
                  const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                  const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError);
        bool upgrade(const std::string& url,
                     const std::string& protocols,
                     const std::function<void(bool)>& onUpgradeInitiate,
                     const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&, bool)>& onResponseReceived,
                     const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError);
        bool sendFile(const std::string& file,
                      const std::function<void(int errnum)>& onStatus,
                      const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                      const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError);
        MasterRequest& sendHeader();
        MasterRequest& sendFragment(const char* chunk, std::size_t chunkLen);
        MasterRequest& sendFragment(const std::string& data);
        bool end(const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onResponseReceived,
                 const std::function<void(const std::shared_ptr<Request>&, const std::string&)>& onResponseParseError);

        bool requestSse(
            const std::string& url,
            const std::function<std::size_t(const std::string& event, const std::string& id, const std::string& data)>& onServerSentEvent);

    private:
        bool initiate(const std::shared_ptr<MasterRequest>& request);

        friend class commands::SendFileCommand;
        friend class commands::SendFragmentCommand;
        friend class commands::SendHeaderCommand;
        friend class commands::UpgradeCommand;
        friend class commands::EndCommand;
        friend class commands::SseCommand;

        bool executeSendFile(const std::string& file, const std::function<void(int)>& onStatus);
        bool executeUpgrade(const std::string& url, const std::string& protocols, const std::function<void(bool)>& onStatus);
        bool executeSendHeader();
        bool executeSendFragment(const char* chunk, std::size_t chunkLen);
        bool executeEnd();
        bool executeSse();

        void requestPrepared(const std::shared_ptr<MasterRequest>& request);
        void requestDelivered();

        void deliverResponse(const std::shared_ptr<MasterRequest>& request, const std::shared_ptr<Response>& response);
        void deliverResponseParseError(const std::shared_ptr<MasterRequest>& request, const std::string& message);

        void onSourceConnect(core::pipe::Source* source) override;
        void onSourceData(const char* chunk, std::size_t chunkLen) override;
        void onSourceEof() override;
        void onSourceError(int errnum) override;

        std::list<RequestCommand*> requestCommands;

        std::size_t contentLengthSent = 0;

        std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)> onResponseReceived;
        std::function<void(const std::shared_ptr<Request>&, const std::string& message)> onResponseParseError;

        friend class SocketContext;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_REQUEST_H
