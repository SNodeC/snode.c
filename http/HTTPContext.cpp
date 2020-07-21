#include "HTTPContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <tuple>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "WebApp.h"
#include "httputils.h"


HTTPContext::HTTPContext(const WebApp& webApp, SocketConnectionBase* connectedSocket)
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
              request.httpVersion = httpVersion;
          },
          [this](const std::string& field, const std::string& value) -> void {
              if (request.requestHeader[field].empty()) {
                  VLOG(1) << "++ Header (insert): " << field << " = " << value;
                  request.requestHeader[field] = value;
              } else {
                  VLOG(1) << "++ Header (append): " << field << " = " << value;
                  request.requestHeader[field] += "," + value;
              }
          },
          [this](const std::string& name, const std::string& value) -> void {
              VLOG(1) << "++ Cookie: " << name << " = " << value;
              request.requestCookies[name] = value;
          },
          [this](char* body, size_t contentLength) -> void {
              VLOG(1) << "++ Body: " << contentLength;
              request.body = body;
              request.contentLength = contentLength;
          },
          [this](void) -> void {
              VLOG(1) << "++ Parsed ++";
              this->requestReady();
          },
          [this](int status, [[maybe_unused]] const std::string& reason) -> void {
              VLOG(1) << "++ Error: " << status << " : " << reason;
              response.responseStatus = status;
              response.send(reason);
              this->connectedSocket->end();
          }) {
    this->reset();
}


void HTTPContext::onReadError(int errnum) {
    response.stop();

    if (errnum != 0 && errnum != ECONNRESET) {
        PLOG(ERROR) << "Connection: read";
    }
}


void HTTPContext::onWriteError(int errnum) {
    response.stop();

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
}


void HTTPContext::end() {
    connectedSocket->end();
}


void HTTPContext::reset() {
    this->requestInProgress = false;
    parser.reset();
    request.reset();
    response.reset();
}
