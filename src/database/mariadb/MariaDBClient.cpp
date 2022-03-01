#include "MariaDBClient.h"

#include "log/Logger.h"

using namespace std;
using namespace net;

namespace database::mariadb {
    MariaDBClient::MariaDBClient()
        : _connection{nullptr}
        , _result{nullptr}
        , _status{0}
        , _connected{false}
        , _queue{} {
        std::string hostname = "localhost";
        std::string username = "rathalin";
        std::string password = "rathalin";
        unsigned int port = 3306;
        std::string socket_name = "/run/mysqld/mysqld.sock";
        std::string db_name = "snodec";
        unsigned int flags = 0;

        mysql_init(&_mysql);
        mysql_options(&_mysql, MYSQL_OPT_NONBLOCK, 0);

        _status = mysql_real_connect_start(
            &_connection, &_mysql, hostname.c_str(), username.c_str(), password.c_str(), db_name.c_str(), port, socket_name.c_str(), flags);
        int fd{mysql_get_socket(&_mysql)};
        if (!printDBError()) {
            ConnectEventReceiver::enable(fd);
        }
    }

    void MariaDBClient::connectEvent() {
        VLOG(0) << "MariaDBClient::connectEvent()";
        _connected = true;
        int fd{mysql_get_socket(&_mysql)};
        VLOG(0) << "File Descriptor: " << fd;
        // Check connection -> disable connectEventReceiver, enable Read, Write
        _status = mysql_real_connect_cont(&_connection, &_mysql, _status);
        if (_status == 0) {
            ConnectEventReceiver::disable();
            ReadEventReceiver::enable(fd);
            WriteEventReceiver::enable(fd);
        }
    }

    void MariaDBClient::writeEvent() {
        if (_connected && !_queue.empty()) {
            int err{0};
            int status{0};
            VLOG(0) << "MariaDBClient::writeEvent() -> " << _queue.front();
            if (_connection == nullptr) {
                VLOG(0) << "Connection null";
            }
            mysql_query_start(&status, _connection, _queue.front().c_str());
            _queue.pop();
            VLOG(0) << "Status: " << status << ", Error: " << err;
            printDBError();
        }
    }

    void MariaDBClient::readEvent() {
        VLOG(0) << "MariaDBClient::readEvent()";
        _result = mysql_use_result(_connection);
        if (_result != nullptr) {
            while (MYSQL_ROW row = mysql_fetch_row(_result)) {
                string id{row[0]};
                string firstname{row[1]};
                string lastname{row[2]};
                string title{row[3]};
                string out{};
                out += id + " | ";
                if (title.length() > 0) {
                    out += title + " ";
                }
                out += firstname + " " + lastname + " ";
                VLOG(0) << out;
            }
            mysql_free_result(_result);
        } else {
            printDBError();
            VLOG(0) << "No Results";
        }
    }

    void MariaDBClient::unobserved() {
        VLOG(0) << "MariaDBClient::unobserved()";
        // Close mariadb connection
    }

    void MariaDBClient::query(string query) {
        _queue.push(query);
        VLOG(0) << "Added to queue: " << query;
    }

    bool MariaDBClient::printDBError() {
        string connection_error{mysql_error(&_mysql)};
        if (connection_error.length() > 0) {
            VLOG(0) << connection_error;
            return true;
        }
        return false;
    }
} // namespace database::mariadb
