#include "database/mariadb/command/MariaDBQueryCommand.h"

namespace database::mariadb::command {
    MariaDBQueryCommand::MariaDBQueryCommand(std::shared_ptr<MYSQL> mysql, std::string query, const std::function<void()>& onComplete)
        : MariaDBCommand{onComplete}
        , mysql{mysql}
        , query{query} {
    }

    int MariaDBQueryCommand::start() {
        return mysql_real_query_start(&error, mysql.get(), query.c_str(), query.length());
    }

    int MariaDBQueryCommand::cont(int& status) {
        return mysql_real_query_cont(&error, mysql.get(), status);
    }
} // namespace database::mariadb::command
