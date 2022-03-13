#ifndef MARIADBCLIENT_H
#define MARIADBCLIENT_H

#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/eventreceiver/WriteEventReceiver.h"
#include "database/mariadb/MariaDBConnector.h"

#include <mysql.h>
#include <queue>
#include <string>

using namespace core;
using namespace core::eventreceiver;

namespace database::mariadb {
    enum SQLMode { None, Query, FetchRow };

    class MariaDBClient
        : public ReadEventReceiver
        , public WriteEventReceiver {
    public:
        MariaDBClient();
        void query(std::string);

    protected:
        void writeEvent() override;
        void readEvent() override;
        void unobservedEvent() override;

    private:
        MariaDBConnector connector;
        MYSQL mysql;
        MYSQL* connection;
        int status;
        int error;
        SQLMode mode;
        MYSQL_RES* result;
        MYSQL_ROW row;
        std::queue<std::string> queue;
        // void onConnect();
        bool printDBError();
        void checkStatus();
        void handleQuery();
    };
} // namespace database::mariadb

#endif // MARIADBCLIENT_H
