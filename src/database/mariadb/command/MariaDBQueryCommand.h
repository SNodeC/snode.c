#ifndef MARIA_DB_QUERY_COMMAND_H
#define MARIA_DB_QUERY_COMMAND_H

#include "database/mariadb/MariaDBCommand.h"

#include <functional>
#include <memory>
#include <mysql.h>
#include <string>

namespace database::mariadb::command {
    class MariaDBQueryCommand : public MariaDBCommand {
    public:
        MariaDBQueryCommand(std::shared_ptr<MYSQL> mysql, std::string query, const std::function<void()>& onComplete);
        virtual int start() override;
        virtual int cont(int& status) override;
        int error;

    private:
        std::shared_ptr<MYSQL> mysql;
        std::string query;
    };
} // namespace database::mariadb::command
#endif // MARIA_DB_QUERY_COMMAND_H