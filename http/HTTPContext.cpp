#include "HTTPContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <filesystem>
#include <numeric>
#include <tuple>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPStatusCodes.h"
#include "Logger.h"
#include "MimeTypes.h"
#include "WebApp.h"
#include "file/FileReader.h"
#include "httputils.h"


HTTPContext::HTTPContext(const WebApp& webApp, SocketConnectionBase* connectedSocket)
    : connectedSocket(connectedSocket)
    , webApp(webApp)
    , response(this)
    , parser(
          [this](std::string& method, std::string& originalUrl, std::string& httpVersion) -> void {
              std::cout << "++ Request: " << method << " " << originalUrl << " " << httpVersion << std::endl;
              request.method = method;
              request.originalUrl = originalUrl;
              request.path = httputils::str_split_last(originalUrl, '/').first;
              if (request.path.empty()) {
                  request.path = "/";
              }
              request.httpVersion = httpVersion;
          },
          [this](const std::string& field, const std::string& value) -> void {
              std::cout << "++ Header: " << field << " = " << value << std::endl;
              if (request.requestHeader[field].empty()) {
                  request.requestHeader[field] = value;
              } else {
                  request.requestHeader[field] += "," + value;
              }
          },
          [this](const std::string& name, const std::string& value) -> void {
              std::cout << "++ Cookie: " << name << " = " << value << std::endl;
              request.requestCookies[name] = value;
          },
          [this](char* body, size_t contentLength) -> void {
              request.body = body;
              request.contentLength = contentLength;
          },
          [this](void) -> void {
              std::cout << "++ Parsed ++" << std::endl;
              this->requestReady();
          },
          [this](int status, [[maybe_unused]] const std::string& reason) -> void {
              std::cout << "++ Error: " << status << " : " << reason << std::endl;
              response.responseStatus = 403;
              this->end();
              this->connectedSocket->end();
          }) {
    this->prepareForRequest();
}


void HTTPContext::stopFileReader() {
    if (fileReader != nullptr) {
        fileReader->stop();
        fileReader = nullptr;
    }
}


void HTTPContext::onReadError(int errnum) {
    stopFileReader();

    if (errnum != 0 && errnum != ECONNRESET) {
        PLOG(ERROR) << "Connection: read";
    }
}


void HTTPContext::onWriteError(int errnum) {
    stopFileReader();

    if (errnum != 0 && errnum != ECONNRESET) {
        PLOG(ERROR) << "Connection write:";
    }
}


void HTTPContext::receiveData(const char* junk, size_t junkLen) {
    if (!requestInProgress) {
        parser.parse(junk, junkLen);
    }
}


void HTTPContext::requestReady() {
    this->requestInProgress = true;

    webApp.dispatch(request, response);
}


void HTTPContext::enqueue(const char* buf, size_t len) {
    connectedSocket->enqueue(buf, len);

    if (headerSend) {
        sendLen += len;
        if (sendLen == response.contentLength) {
            if (request.requestHeader.find("connection") != request.requestHeader.end()) {
                if (request.requestHeader.find("connection")->second == "Close") {
                    connectedSocket->end();
                } else {
                    prepareForRequest();
                }
            } else {
                connectedSocket->end();
            }
        }
    }
}


void HTTPContext::enqueue(const std::string& str) {
    this->enqueue(str.c_str(), str.size());
}


void HTTPContext::send(const char* buffer, int size) {
    response.responseHeader.insert({"Content-Type", "application/octet-stream"});
    response.responseHeader.insert({"Content-Length", std::to_string(size)});

    this->sendHeader();
    this->enqueue(buffer, size);
}


void HTTPContext::send(const std::string& buffer) {
    response.responseHeader.insert({"Content-Type", "text/html; charset=utf-8"});

    this->send(buffer.c_str(), buffer.size());
}


void HTTPContext::sendFile(const std::string& file, const std::function<void(int ret)>& onError) {
    std::string absolutFileName = webApp.getRootDir() + file;

    if (std::filesystem::exists(absolutFileName)) {
        std::error_code ec;
        absolutFileName = std::filesystem::canonical(absolutFileName);

        if (absolutFileName.rfind(webApp.getRootDir(), 0) == 0 && std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
            response.responseHeader.insert({"Content-Type", MimeTypes::contentType(absolutFileName)});
            response.responseHeader.insert_or_assign("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName)));
            response.responseHeader.insert({"Last-Modified", httputils::file_mod_http_date(absolutFileName)});
            this->sendHeader();

            fileReader = FileReader::read(
                absolutFileName,
                [this](char* data, int length) -> void {
                    this->enqueue(data, length);
                },
                [this, onError](int err) -> void {
                    if (onError) {
                        onError(err);
                    }
                    if (err != 0) {
                        connectedSocket->end();
                    }
                });
        } else {
            response.responseStatus = 403;
            errno = EACCES;
            this->end();
            if (onError) {
                onError(EACCES);
            }
        }
    } else {
        response.responseStatus = 404;
        errno = ENOENT;
        this->end();
        if (onError) {
            onError(ENOENT);
        }
    }
}


void HTTPContext::sendHeader() {
    if (!headerSend) {
        this->enqueue("HTTP/1.1 " + std::to_string(response.responseStatus) + " " + HTTPStatusCode::reason(response.responseStatus) +
                      "\r\n");
        this->enqueue("Date: " + httputils::to_http_date() + "\r\n");

        response.responseHeader.insert({"Cache-Control", "public, max-age=0"});
        response.responseHeader.insert({"Accept-Ranges", "bytes"});
        response.responseHeader.insert({"X-Powered-By", "snode.c"});

        for (const std::pair<const std::string, std::string>& header : response.responseHeader) {
            this->enqueue(header.first + ": " + header.second + "\r\n");
        }

        for (const std::pair<const std::string, Response::ResponseCookie>& cookie : response.responseCookies) {
            std::string cookieString =
                std::accumulate(cookie.second.options.begin(), cookie.second.options.end(), cookie.first + "=" + cookie.second.value,
                                [](const std::string& str, const std::pair<const std::string&, const std::string&> option) -> std::string {
                                    return str + "; " + option.first + (!option.second.empty() ? "=" + option.second : "");
                                });
            this->enqueue("Set-Cookie: " + cookieString + "\r\n");
        }

        this->enqueue("\r\n");

        headerSend = true;

        response.contentLength = std::stoi(response.responseHeader.find("Content-Length")->second);
    }
}


void HTTPContext::end() {
    response.responseHeader.insert({"Content-Length", "0"});
    this->sendHeader();
    this->prepareForRequest();
}


void HTTPContext::prepareForRequest() {
    this->headerSend = false;
    this->sendLen = 0;
    this->stopFileReader();
    this->fileReader = nullptr;
    this->requestInProgress = false;

    parser.reset();
    request.reset();
    response.reset();
}
