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

        WebApp(const std::string& rootDir, const std::string& cert, const std::string& key, const std::string& password,
               const Router& router)
            : ::WebApp(rootDir)
            , cert(cert)
            , key(key)
            , password(password) {
            this->setRoute(router.getRoute());
            this->setMountPoint(router.getMountPoint());
        }

        WebApp(const WebApp& webApp)
            : ::WebApp(webApp.getRootDir())
            , cert(webApp.cert)
            , key(webApp.key)
            , password(webApp.password) {
            this->setRoute(webApp.getRoute());
            this->setMountPoint(webApp.getMountPoint());
        }

        WebApp& operator=(const ::WebApp& webApp) {
            this->setRootDir(webApp.getRootDir());
            this->setRoute(webApp.getRoute());
            this->setMountPoint(webApp.getMountPoint());

            return *this;
        }

        void listen(int port, const std::function<void(int err)>& onError = nullptr) override;

    private:
        static void start() {
        }

        static void stop() {
        }

        std::string cert;
        std::string key;
        std::string password;
    };

} // namespace tls

#endif // TLSWEBAPP_H
