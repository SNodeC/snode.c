#ifndef LEGACYHTTPSERVER_H
#define LEGACYHTTPSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <netinet/in.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class Request;
class Response;

namespace legacy {

    class HTTPServer {
    public:
        explicit HTTPServer(const std::function<void(Request& req, Response& res)>& onRequest);

        HTTPServer& operator=(const HTTPServer& webApp) = delete;

        void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr);

    protected:
        std::function<void(Request& req, Response& res)> onRequest;
    };

} // namespace legacy

#endif // LEGACYHTTPSERVER_H
