#ifndef MARIA_DB_FETCH_ROW_COMMAND_H
#define MARIA_DB_FETCH_ROW_COMMAND_H

#include "database/mariadb/MariaDBCommand.h"

#include <functional>
#include <string>

namespace database::mariadb::command {
    class MariaDBFetchRowCommand : public MariaDBCommand {
    public:
        MariaDBFetchRowCommand(std::shared_ptr<MYSQL> mysql, MYSQL_RES* result, const std::function<void()>& onRowResult);
        int start() override;
        int cont(int& status) override;
        void onComplete() override;
        int error;

    private:
        MYSQL_ROW row;
        MYSQL_RES* result;
        std::function<void()> onRowResult;
    };
} // namespace database::mariadb::command
#endif // MARIA_DB_FETCH_ROW_COMMAND_H