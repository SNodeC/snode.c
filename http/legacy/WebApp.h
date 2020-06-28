#ifndef LEGACYWEBAPP_H
#define LEGACYWEBAPP_H

#include "../WebApp.h"


namespace legacy {

    class WebApp : public ::WebApp {
    public:
        explicit WebApp(const std::string& rootDir)
            : ::WebApp(rootDir){};

        WebApp(const std::string& rootDir, const ::WebApp& webApp)
            : ::WebApp(rootDir) {
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
    };

} // namespace legacy

#endif // LEGACYWEBAPP_H
