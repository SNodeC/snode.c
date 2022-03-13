#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/eventreceiver/WriteEventReceiver.h"

#include <functional>
#include <mysql.h>

using namespace core;
using namespace core::eventreceiver;

namespace database::mariadb {
    class MariaDBConnector
        // : public ExceptionalConditionEventReceiver
        : public ReadEventReceiver
        , public WriteEventReceiver {
    public:
        MariaDBConnector();
        MYSQL getMYSQL();
        MYSQL* getConnection();
        int getStatus();
        void setOnConnect(const std::function<void()>&);
        void connect();

    protected:
        void writeEvent() override;
        void readEvent() override;
        // void outOfBandEvent() override;
        void unobservedEvent() override;

    private:
        MYSQL mysql;
        MYSQL* connection;
        int status;
        std::function<void()> onConnect;
        void checkStatus();
        bool printDBError();
    };
} // namespace database::mariadb
