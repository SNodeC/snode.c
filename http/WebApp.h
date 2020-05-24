#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Router.h"
#include "Request.h"
#include "Response.h"

class SocketConnectionInterface;

class WebApp : public Router
{
private:
    WebApp(const std::string& serverRoot);

public:
    ~WebApp();

    static WebApp& instance(const std::string& serverRoot = "./");

    void listen(int port);
    void listen(int port, const std::function<void (int err)>& onError);


    void sslListen(int port);
    void sslListen(int port, const std::function<void (int err)>& onError);

    void stop();
    void sslStop();

    void destroy();

    std::string& getRootDir() {
        return rootDir;
    }

    void serverRoot(std::string rootDir) {
        this->rootDir = rootDir;
        if (this->rootDir.back() != '/') {
            this->rootDir += '/';
        }
    }

    std::string rootDir;
};

#endif // HTTPSERVER_H
