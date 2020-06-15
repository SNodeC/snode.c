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
    explicit WebApp(const std::string& rootDir = "./");

public:
    void listen(int port, const std::function<void(int err)>& onError = nullptr);
    void tlsListen(int port, const std::string& cert, const std::string& key, const std::string& password,
                   const std::function<void(int err)>& onError = nullptr);

    static void start(int argc, char** argv);
    static void stop();

    [[nodiscard]] const std::string& getRootDir() const {
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
