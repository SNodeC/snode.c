#ifndef WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEINTERFACE_H
#define WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEINTERFACE_H

namespace web::http::server {

    class SocketContextUpgradeFactory;

} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    extern "C" {
        class SocketContextUpgradeInterface {
        public:
            virtual ~SocketContextUpgradeInterface() = default;

            virtual web::http::server::SocketContextUpgradeFactory* create() = 0;
        };
    }

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEINTERFACE_H
