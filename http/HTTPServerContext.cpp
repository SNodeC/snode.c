#include "HTTPServerContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "WebApp.h"
#include "httputils.h"
#include "socket/SocketConnectionBase.h"

HTTPServerContext::HTTPServerContext(const WebApp& webApp, SocketConnectionBase* connectedSocket)
    : connectedSocket(connectedSocket)
    , webApp(webApp)
    , response(this)
    , parser(
          [this](std::string& method, std::string& originalUrl, std::string& httpVersion) -> void {
              VLOG(1) << "++ Request: " << method << " " << originalUrl << " " << httpVersion;
              request.method = method;
              request.originalUrl = originalUrl;
              request.path = httputils::str_split_last(originalUrl, '/').first;
              if (request.path.empty()) {
                  request.path = "/";
              }
              request.url = httputils::str_split_last(originalUrl, '?').first;
              request.httpVersion = httpVersion;
          },
          [this](const std::map<std::string, std::string>& header, const std::map<std::string, std::string>& cookies) -> void {
              VLOG(1) << "++ Header:";
              request.requestHeader = &header;
              VLOG(1) << "++ Cookies";
              request.requestCookies = &cookies;
          },
          [this](char* content, size_t contentLength) -> void {
              VLOG(1) << "++ Content: " << contentLength;
              request.body = content;
              request.contentLength = contentLength;
          },
          [this](void) -> void {
              VLOG(1) << "++ Parsed ++";
              this->requestReady();
          },
          [this](int status, [[maybe_unused]] const std::string& reason) -> void {
              VLOG(1) << "++ Error: " << status << " : " << reason;
              response.status(status).send(reason);
              this->connectedSocket->end();
          }) {
    this->requestCompleted();
}

void HTTPServerContext::receiveRequestData(const char* junk, size_t junkLen) {
    if (!requestInProgress) {
        parser.parse(junk, junkLen);
    } else {
        terminateConnection();
    }
}

void HTTPServerContext::onReadError(int errnum) {
    response.stop();

    if (errnum != 0 && errnum != ECONNRESET) {
        PLOG(ERROR) << "Connection: read";
    }
}

void HTTPServerContext::sendResponseData(const char* buf, size_t len) {
    connectedSocket->enqueue(buf, len);
}

void HTTPServerContext::onWriteError(int errnum) {
    response.stop();

    if (errnum != 0 && errnum != ECONNRESET) {
        PLOG(ERROR) << "Connection write";
    }
}

void HTTPServerContext::requestReady() {
    this->requestInProgress = true;

    webApp.dispatch(request, response);
}

void HTTPServerContext::requestCompleted() {
    this->requestInProgress = false;
    request.reset();
    response.reset();
}

void HTTPServerContext::terminateConnection() {
    connectedSocket->end();
}
