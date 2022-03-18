#ifndef MARIA_DB_EXECUTOR
#define MARIA_DB_EXECUTOR

#include "core/eventreceiver/ExceptionalConditionEventReceiver.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/eventreceiver/WriteEventReceiver.h"
#include "database/mariadb/MariaDBCommand.h"
#include "database/mariadb/MariaDBConnectionDetails.h"

#include <functional>
#include <vector>
#include <string>

namespace database::mariadb {
    class MariaDBExecutor
        : core::eventreceiver::ReadEventReceiver
        , core::eventreceiver::WriteEventReceiver
        , core::eventreceiver::ExceptionalConditionEventReceiver {
    public:
        MariaDBExecutor();
        ~MariaDBExecutor();
        void connect(MariaDBConnectionDetails details, const std::function<void()>& onConnect);
        void query(std::string sql, const std::function<void(/*std::shared_ptr<std::vector<std::vector<std:string>>> result*/)>& onResult);
        void fetchRow(MYSQL_RES* result, const std::function<void()>& onResult);

    protected:
        void writeEvent() override;
        void readEvent() override;
        void outOfBandEvent() override;
        void unobservedEvent() override;

    private:
        std::shared_ptr<MYSQL> mysql;
        std::shared_ptr<MariaDBCommand> currentCommand;
        int currentStatus;
        std::function<void()> onQueryResult;
        void enableAndSuspendEventReceivers(int& fd);
        void manageEventReceivers();
        void checkStatus();
        void executeCurrentCommand();
    };
} // namespace database::mariadb

#endif // MARIA_DB_EXECUTOR
