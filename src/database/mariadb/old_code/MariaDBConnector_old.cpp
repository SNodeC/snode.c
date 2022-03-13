#include "MariaDBConnector.h"

#include "log/Logger.h"

#include <string>

using namespace std;
using namespace core;
using namespace core::eventreceiver;

namespace database::mariadb {
    MariaDBConnector::MariaDBConnector()
        : ReadEventReceiver{"MariaDBConnector"}
        , WriteEventReceiver{"MariaDBConnector"}
        , mysql{}
        , connection{nullptr}
        , status{0} {
    }

    void MariaDBConnector::setOnConnect(const function<void()>& onConnect) {
        this->onConnect = onConnect;
    }

    void MariaDBConnector::connect() {
        string hostname = "localhost";
        string username = "rathalin";
        string password = "rathalin";
        unsigned int port = 3306;
        string socket_name = "/run/mysqld/mysqld.sock";
        string db_name = "snodec";
        unsigned int flags = 0;

        mysql_init(&mysql);
        mysql_options(&mysql, MYSQL_OPT_NONBLOCK, 0);

        status = mysql_real_connect_start(
            &connection, &mysql, hostname.c_str(), username.c_str(), password.c_str(), db_name.c_str(), port, socket_name.c_str(), flags);
        if (status == 0) {
            VLOG(0) << "Already connected";
            onConnect();
        } else {
            int fd{mysql_get_socket(&mysql)};
            VLOG(0) << "MariaDBConnector fd: " << fd;
            if (!printDBError()) {
                ReadEventReceiver::enable(fd);
                WriteEventReceiver::enable(fd);
                ReadEventReceiver::suspend();
                WriteEventReceiver::suspend();
                checkStatus();
            }
        }
    }

    void MariaDBConnector::readEvent() {
        VLOG(0) << "MariaDBConnector::readEvent";
        checkStatus();
    }

    void MariaDBConnector::writeEvent() {
        VLOG(0) << "MariaDBConnector::writeEvent";
        checkStatus();
    }

    void MariaDBConnector::unobservedEvent() {
        VLOG(0) << "MariaDBConnector::unobservedEvent()";
    }

    void MariaDBConnector::checkStatus() {
        VLOG(0) << "MariaDBConnector::status " << status;
        if (status == 0) {
            ReadEventReceiver::disable();
            WriteEventReceiver::disable();
            onConnect();
            return;
        }
        WriteEventReceiver::suspend();
        ReadEventReceiver::suspend();
        if (status & MYSQL_WAIT_READ) {
            ReadEventReceiver::resume();
        }
        if (status & MYSQL_WAIT_WRITE) {
            WriteEventReceiver::resume();
        }
        status = mysql_real_connect_cont(&connection, &mysql, status);
        VLOG(0) << "Status: " << status;
        printDBError();
    }

    bool MariaDBConnector::printDBError() {
        string connection_error{mysql_error(&mysql)};
        if (connection_error.length() > 0) {
            VLOG(0) << connection_error;
            return true;
        }
        return false;
    }

    MYSQL MariaDBConnector::getMYSQL() {
        return mysql;
    }

    MYSQL* MariaDBConnector::getConnection() {
        return connection;
    }

    int MariaDBConnector::getStatus() {
        return status;
    }
} // namespace database::mariadb
