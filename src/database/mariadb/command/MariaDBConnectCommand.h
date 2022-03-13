#ifndef MARIA_DB_CONNECT_COMMAND_H
#define MARIA_DB_CONNECT_COMMAND_H

#include "database/mariadb/MariaDBCommand.h"
#include "database/mariadb/MariaDBConnectionDetails.h"

#include <functional>
#include <memory>
#include <mysql.h>

namespace database::mariadb::command {
    class MariaDBConnectCommand : public MariaDBCommand {
    public:
        MariaDBConnectCommand(std::shared_ptr<MYSQL> mysql,
                              database::mariadb::MariaDBConnectionDetails details,
                              const std::function<void()>& onConnect);
        virtual int start() override;
        virtual int cont(int&) override;

    private:
        std::unique_ptr<MYSQL> ret;
        std::shared_ptr<MYSQL> mysql;
        database::mariadb::MariaDBConnectionDetails details;
    };
} // namespace database::mariadb::command

#endif // MARIA_DB_CONNECT_COMMAND_H
