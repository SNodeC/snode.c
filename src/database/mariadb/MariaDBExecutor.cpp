#include "database/mariadb/MariaDBExecutor.h"

#include "database/mariadb/MariaDBCommand.h"
#include "database/mariadb/command/MariaDBConnectCommand.h"
#include "log/Logger.h"

using namespace std;
using namespace core::eventreceiver;
using namespace database::mariadb::command;

namespace database::mariadb {
    MariaDBExecutor::MariaDBExecutor()
        : ReadEventReceiver{"MariaDBExecutor"}
        , WriteEventReceiver{"MariaDBExecutor"}
        , ExceptionalConditionEventReceiver{"MariaDBExecutor"}
        , mysql{}
        , currentCommand{}
        , currentStatus{-1} {
    }

    MariaDBExecutor::~MariaDBExecutor() {
    }

    void MariaDBExecutor::connect(MariaDBConnectionDetails details, const std::function<void()>& onConnect) {
        mysql = make_shared<MYSQL>();
        mysql_init(mysql.get());
        mysql_options(mysql.get(), MYSQL_OPT_NONBLOCK, 0);
        currentCommand.reset(new MariaDBConnectCommand{mysql, details, onConnect});
        currentStatus = currentCommand->start();
        int fd{mysql_get_socket(mysql.get())};
        enableAndSuspendEventReceivers(fd);
        if (currentStatus != 0) {
            manageEventReceivers();
        } else {
            onConnect();
        }
    }

    void MariaDBExecutor::query(string sql) {
        
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
        WriteEventReceiver::suspend();
        ReadEventReceiver::suspend();
        ExceptionalConditionEventReceiver::suspend();
        if (currentStatus & MYSQL_WAIT_READ) {
            ReadEventReceiver::resume();
        }
        if (currentStatus & MYSQL_WAIT_WRITE) {
            WriteEventReceiver::resume();
        }
        if (currentStatus & MYSQL_WAIT_EXCEPT) {
            ExceptionalConditionEventReceiver::resume();
        }
    }

    void MariaDBExecutor::checkStatus() {
        currentStatus = currentCommand->cont(currentStatus);
        if (currentStatus != 0) {
            manageEventReceivers();
        } else {
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
