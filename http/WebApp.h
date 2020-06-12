#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"
#include "Router.h"


class WebApp : public Router {
public:
    WebApp(const std::string& rootDir = "./");

public:
    ~WebApp();

    void listen(int port, const std::function<void(int err)>& onError = 0);
    void sslListen(int port, const std::string& cert, const std::string& key, const std::string& password,
                   const std::function<void(int err)>& onError = 0);

    static void start();
    static void stop();

    const std::string& getRootDir() {
        return rootDir;
    }

    void serverRoot(const std::string& rootDir) {
        this->rootDir = rootDir;
        if (this->rootDir.back() == '/') {
            this->rootDir.pop_back();
        }
    }

private:
    std::string rootDir;
};

#endif // HTTPSERVER_H
