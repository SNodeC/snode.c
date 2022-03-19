#include "database/mariadb/command/MariaDBQueryCommand.h"

#include "log/Logger.h"

namespace database::mariadb::command {
    MariaDBQueryCommand::MariaDBQueryCommand(std::shared_ptr<MYSQL> mysql, std::string sql, const std::function<void()>& onQueryComplete)
        : MariaDBCommand{mysql}
        , error{0}
        , sql{sql}
        , onQueryComplete{onQueryComplete} {
        VLOG(0) << "MariaDBQueryCommand with query: " << sql;
    }

    int MariaDBQueryCommand::start() {
        int status{mysql_real_query_start(&error, mysql.get(), sql.c_str(), sql.length())};
        if (error != 0) {
            VLOG(0) << "mysql_real_query_start() returns error";
        }
        return status;
    }

    int MariaDBQueryCommand::cont(int& status) {
        status = mysql_real_query_cont(&error, mysql.get(), status);
        if (error != 0) {
            VLOG(0) << "mysql_real_query_cont() returns error";
        }
        return status;
    }

    void MariaDBQueryCommand::onComplete() {
        onQueryComplete();
    }
} // namespace database::mariadb::command
