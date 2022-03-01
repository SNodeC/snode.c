#ifndef MARIADBCLIENT_H
#define MARIADBCLIENT_H

#include "net/ConnectEventReceiver.h"
#include "net/Descriptor.h"
#include "net/ReadEventReceiver.h"
#include "net/WriteEventReceiver.h"

#include <mysql.h>
#include <queue>
#include <string>

using namespace net;

namespace database::mariadb {
    class MariaDBClient
        : public ConnectEventReceiver
        , public ReadEventReceiver
        , public WriteEventReceiver {
    public:
        MariaDBClient();
        void query(std::string);

    protected:
        void connectEvent() override;
        void writeEvent() override;
        void readEvent() override;
        void unobserved() override;

    private:
        MYSQL _mysql;
        MYSQL* _connection;
        MYSQL_RES* _result;
        int _status;
        bool _connected;
        std::queue<std::string> _queue;
        bool printDBError();
    };
} // namespace database::mariadb

#endif // MARIADBCLIENT_H
