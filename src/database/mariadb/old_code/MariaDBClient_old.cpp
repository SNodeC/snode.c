#include "MariaDBClient.h"

#include "log/Logger.h"

using namespace std;
using namespace core;

namespace database::mariadb {
    MariaDBClient::MariaDBClient()
        : ReadEventReceiver{"MariaDBClient"}
        , WriteEventReceiver{"MariaDBClient"}
        , connector{}
        , mysql{}
        , connection{}
        , status{}
        , error{-1}
        , mode{SQLMode::None}
        , result{nullptr}
        , queue{} {
        connector.setOnConnect([this]() -> void {
            VLOG(0) << "MariaDBClient::onConnect()";
            int fd{mysql_get_socket(&mysql)};
            ReadEventReceiver::enable(fd);
            WriteEventReceiver::enable(fd);
            ReadEventReceiver::suspend();
            WriteEventReceiver::suspend();
            handleQuery();
        });
        connector.connect();
        mysql = connector.getMYSQL();
        connection = connector.getConnection();
        status = connector.getStatus();
    }

    void MariaDBClient::writeEvent() {
        VLOG(0) << "MariaDBClient::writeEvent()";
        checkStatus();
    }

    void MariaDBClient::readEvent() {
        VLOG(0) << "MariaDBClient::readEvent()";
        checkStatus();
    }

    void MariaDBClient::unobservedEvent() {
        VLOG(0) << "MariaDBClient::unobservedEvent()";
        // Close mariadb connection
    }

    void MariaDBClient::query(string query) {
        queue.push(query);
        VLOG(0) << "Added to queue: " << query;
        handleQuery();
    }

    // void onConnect() {
    //     VLOG(0) << "MariaDBClient::onConnect()";
    // }

    bool MariaDBClient::printDBError() {
        string connection_error{mysql_error(&mysql)};
        if (connection_error.length() > 0) {
            VLOG(0) << connection_error;
            return true;
        }
        return false;
    }

    void MariaDBClient::checkStatus() {
        if (status == 0) {
            switch (mode) {
                case SQLMode::None: {
                    break;
                }
                case SQLMode::Query: {
                    result = mysql_use_result(&mysql);
                    mode = SQLMode::FetchRow;
                    status = mysql_fetch_row_start(&row, result);
                    ReadEventReceiver::resume();
                    WriteEventReceiver::resume();
                    break;
                }
                case SQLMode::FetchRow: {
                    if (!result) {
                        mode = SQLMode::None;
                    } else {
                        VLOG(0) << row[0] << " " << row[1];
                    }
                    break;
                }
            }
            return;
        }
        if (status & MYSQL_WAIT_READ) {
            ReadEventReceiver::resume();
            WriteEventReceiver::suspend();
        } else if (status & MYSQL_WAIT_WRITE) {
            WriteEventReceiver::resume();
            ReadEventReceiver::suspend();
        }
        switch (mode) {
            case SQLMode::None: {
                break;
            }
            case SQLMode::Query: {
                status = mysql_real_query_cont(&error, &mysql, status);
                break;
            }
            case SQLMode::FetchRow: {
                status = mysql_fetch_row_cont(&row, result, status);
                break;
            }
        }
    }

    void MariaDBClient::handleQuery() {
        VLOG(0) << "handleQuery()";
        if (queue.empty())
            return;
        mode = SQLMode::Query;
        // VLOG(0) << "Mode: " << mode;
        // VLOG(0) << "query size: " << queue.size();
        string query{queue.front()};
        queue.pop();
        VLOG(0) << "before mysql_real_query_start";
        VLOG(0) << "MYSQL: " << &mysql;
        VLOG(0) << "error: " << error;
        status = mysql_real_query_start(&error, &mysql, query.c_str(), query.length());
        VLOG(0) << "Status after query start " << status;
        VLOG(0) << "after mysql_real_query_start";
        ReadEventReceiver::resume();
        WriteEventReceiver::resume();
        VLOG(0) << "End of handleQuery()";
    }
} // namespace database::mariadb
