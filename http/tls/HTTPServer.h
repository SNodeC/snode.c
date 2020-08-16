#ifndef TLSHTTPSERVER_H
#define TLSHTTPSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class Request;
class Response;

namespace tls {

    class HTTPServer {
    public:
        explicit HTTPServer(const std::string& cert, const std::string& key, const std::string& password,
                            const std::function<void(Request& req, Response& res)>& onRequest);

        HTTPServer& operator=(const HTTPServer& webApp) = delete;

        void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr);

    protected:
        std::function<void(Request& req, Response& res)> onRequest;

    private:
        std::string cert;
        std::string key;
        std::string password;
    };

} // namespace tls

#endif // TLSHTTPSERVER_H
