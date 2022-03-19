#include "database/mariadb/MariaDBExecutor.h"

#include "database/mariadb/MariaDBCommand.h"
#include "database/mariadb/command/MariaDBConnectCommand.h"
#include "database/mariadb/command/MariaDBFetchRowCommand.h"
#include "database/mariadb/command/MariaDBQueryCommand.h"
#include "log/Logger.h"

using namespace std;
using namespace core::eventreceiver;
using namespace database::mariadb::command;

namespace database::mariadb {
    MariaDBExecutor::MariaDBExecutor()
        : ReadEventReceiver{"MariaDBExecutor"}
        , WriteEventReceiver{"MariaDBExecutor"}
        , ExceptionalConditionEventReceiver{"MariaDBExecutor"}
        , mysql{make_shared<MYSQL>()}
        , currentCommand{}
        , currentStatus{-1} {
    }

    MariaDBExecutor::~MariaDBExecutor() {
    }

    void MariaDBExecutor::connect(MariaDBConnectionDetails details, const function<void()>& onConnect) {
        currentCommand.reset(new MariaDBConnectCommand{mysql, details, onConnect});
        currentStatus = currentCommand->start();
        int fd{mysql_get_socket(mysql.get())};
        enableAndSuspendEventReceivers(fd);
        executeCurrentCommand();
    }

    void MariaDBExecutor::query(string sql, const function<void(/*shared_ptr<vector<vector<string>>> result*/)>& onResult) {
        currentCommand.reset(new MariaDBQueryCommand{mysql, sql, [onResult, this]() -> void {
                                                         VLOG(0) << "Query complete";
                                                         VLOG(0) << mysql_errno(mysql.get()) << " " << mysql_error(mysql.get());
                                                         MYSQL_RES* result{mysql_use_result(mysql.get())};
                                                         if (!result) {
                                                             VLOG(0) << mysql_errno(mysql.get()) << " " << mysql_error(mysql.get());
                                                             VLOG(0) << "mysql_use_result() returns error";
                                                         }
                                                         this->fetchRow(result, onResult);
                                                     }});
        currentStatus = currentCommand->start();

        executeCurrentCommand();
    }

    void MariaDBExecutor::fetchRow(MYSQL_RES* result, const function<void()>& onResult) {
        currentCommand.reset(new MariaDBFetchRowCommand{mysql, result, onResult});

        currentStatus = currentCommand->start();

        executeCurrentCommand();
    }

    void MariaDBExecutor::enableAndSuspendEventReceivers(int& fd) {
        ReadEventReceiver::enable(fd);
        WriteEventReceiver::enable(fd);
        ExceptionalConditionEventReceiver::enable(fd);
        ReadEventReceiver::suspend();
        WriteEventReceiver::suspend();
        ExceptionalConditionEventReceiver::suspend();
    }

    void MariaDBExecutor::manageEventReceivers() {
        VLOG(0) << "ManageEventReceivers 0";

        if (!WriteEventReceiver::isSuspended()) {
            WriteEventReceiver::suspend();
        }
        if (!ReadEventReceiver::isSuspended()) {
            ReadEventReceiver::suspend();
        }
        if (!ExceptionalConditionEventReceiver::isSuspended()) {
            ExceptionalConditionEventReceiver::suspend();
        }
        if (currentStatus & MYSQL_WAIT_READ) {
            VLOG(0) << "ManageEventReceivers 1";
            ReadEventReceiver::resume();
        }
        if (currentStatus & MYSQL_WAIT_WRITE) {
            VLOG(0) << "ManageEventReceivers 2";
            WriteEventReceiver::resume();
        }
        if (currentStatus & MYSQL_WAIT_EXCEPT) {
            VLOG(0) << "ManageEventReceivers 3";
            ExceptionalConditionEventReceiver::resume();
        }
    }

    void MariaDBExecutor::checkStatus() {
        currentStatus = currentCommand->cont(currentStatus);

        VLOG(0) << "Current Status: " << currentStatus;
        if (currentStatus != 0) {
            manageEventReceivers();
        } else {
            VLOG(0) << "Command completed 1";
            currentCommand->onComplete();
        }
    }

    void MariaDBExecutor::executeCurrentCommand() {
        if (currentStatus != 0) {
            manageEventReceivers();
        } else {
            VLOG(0) << "Command completed 2";
            currentCommand->onComplete();
        }
    }

    void MariaDBExecutor::writeEvent() {
        checkStatus();
    }
    void MariaDBExecutor::readEvent() {
        checkStatus();
    }
    void MariaDBExecutor::outOfBandEvent() {
        VLOG(0) << "Out of band Event MariaDBExecutor";
    }
    void MariaDBExecutor::unobservedEvent() {
        VLOG(0) << "Unobserved Event MariaDBExecutor";
    }
} // namespace database::mariadb
