#ifndef MARIA_DB_QUERY_COMMAND_H
#define MARIA_DB_QUERY_COMMAND_H

#include "database/mariadb/MariaDBCommand.h"

#include <functional>
#include <string>

namespace database::mariadb::command {
    class MariaDBQueryCommand : public MariaDBCommand {
    public:
        MariaDBQueryCommand(std::shared_ptr<MYSQL> mysql, std::string sql, const std::function<void()>& onQueryComplete);
        int start() override;
        int cont(int& status) override;
        void onComplete() override;
        int error;

    private:
        std::string sql;
        std::function<void()> onQueryComplete;
    };
} // namespace database::mariadb::command
#endif // MARIA_DB_QUERY_COMMAND_H