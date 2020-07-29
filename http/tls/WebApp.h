#ifndef TLSWEBAPP_H
#define TLSWEBAPP_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "../WebApp.h"

namespace tls {

    class WebApp : public ::WebApp {
    public:
        WebApp(const std::string& rootDir, const std::string& cert, const std::string& key, const std::string& password)
            : ::WebApp(rootDir)
            , cert(cert)
            , key(key)
            , password(password){};

        void listen(int port, const std::function<void(int err)>& onError = nullptr) override;

    private:
        using ::WebApp::start;
        using ::WebApp::stop;

        std::string cert;
        std::string key;
        std::string password;
    };

} // namespace tls

#endif // TLSWEBAPP_H
