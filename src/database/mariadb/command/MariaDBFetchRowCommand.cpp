#include "database/mariadb/command/MariaDBFetchRowCommand.h"

#include "log/Logger.h"

using namespace std;

namespace database::mariadb::command {
    MariaDBFetchRowCommand::MariaDBFetchRowCommand(shared_ptr<MYSQL> mysql, MYSQL_RES* result, const function<void()>& onRowResult)
        : MariaDBCommand{mysql}
        , row{}
        , result{result}
        , onRowResult{onRowResult} {
    }

    int MariaDBFetchRowCommand::start() {
        return mysql_fetch_row_start(&row, result);
    }

    int MariaDBFetchRowCommand::cont(int& status) {
        return mysql_fetch_row_cont(&row, result, status);
    }

    void MariaDBFetchRowCommand::onComplete() {
        if (!row) {
            VLOG(0) << "No result";
            VLOG(0) << mysql_error(mysql.get());
        }
        onRowResult();
    }
} // namespace database::mariadb::command
