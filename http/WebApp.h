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
protected:
    explicit WebApp(const std::string& rootDir = "./");

    virtual void listen(int port, const std::function<void(int err)>& onError = nullptr) = 0;

public:
    static void init(int argc, char* argv[]);
    static void start();
    static void stop();

    [[nodiscard]] const std::string& getRootDir() const {
        return rootDir;
    }

    void setRootDir(const std::string& rootDir) {
        this->rootDir = rootDir;
        if (this->rootDir.back() == '/') {
            this->rootDir.pop_back();
        }
    }

    static void clone(WebApp& dest, const WebApp& src) {
        dest.rootDir = src.rootDir;
        dest.routerRoute = src.routerRoute;
        dest.mountPoint = src.mountPoint;
    }

private:
    std::string rootDir;

    static bool initialized;
};

#endif // HTTPSERVER_H
