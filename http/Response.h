/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RESPONSE_H
#define RESPONSE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class FileReader;
class HTTPServerContext;

class Response {
private:
    explicit Response(HTTPServerContext* httpContext);

public:
    void send(const char* buffer, size_t size);
    void send(const std::string& text);

    void sendFile(const std::string& file, const std::function<void(int err)>& onError = nullptr);
    void download(const std::string& file, const std::function<void(int err)>& onError = nullptr);
    void download(const std::string& file, const std::string& name, const std::function<void(int err)>& onError = nullptr);

    void redirect(const std::string& name);
    void redirect(int status, const std::string& name);

    void sendStatus(int status);
    void end();

    Response& status(int status);
    Response& append(const std::string& field, const std::string& value);
    Response& set(const std::string& field, const std::string& value, bool overwrite = false);
    Response& set(const std::map<std::string, std::string>& headers, bool overwrite = false);
    Response& cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {});
    Response& clearCookie(const std::string& name, const std::map<std::string, std::string>& options = {});
    Response& type(const std::string& type);

    bool headersSent = false;

protected:
    mutable size_t contentLength;

    void enqueue(const char* buf, size_t len);
    void enqueue(const std::string& str);

    void sendHeader();
    void disable();
    void reset();

private:
    class ResponseCookie {
    public:
        ResponseCookie(const std::string& value, const std::map<std::string, std::string>& options)
            : value(value)
            , options(options) {
        }

    protected:
        std::string value;
        std::map<std::string, std::string> options;

        friend class Response;
    };

    HTTPServerContext* httpContext;
    FileReader* fileReader = nullptr;

    size_t contentSent = 0;
    bool headersSentInProgress = false;

    int responseStatus = 0;
    std::map<std::string, std::string> headers;
    std::map<std::string, ResponseCookie> cookies;

    friend class HTTPServerContext;
};

#endif // RESPONSE_H
