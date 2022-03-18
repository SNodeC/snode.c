#ifndef MARIA_DB_CONNECT_COMMAND_H
#define MARIA_DB_CONNECT_COMMAND_H

#include "database/mariadb/MariaDBCommand.h"
#include "database/mariadb/MariaDBConnectionDetails.h"
#include <functional>

namespace database::mariadb::command {
    class MariaDBConnectCommand : public MariaDBCommand {
    public:
        MariaDBConnectCommand(std::shared_ptr<MYSQL> mysql,
                              database::mariadb::MariaDBConnectionDetails details,
                              const std::function<void()>& onConnect);
        int start() override;
        int cont(int&) override;
        void onComplete() override;

    private:
        std::unique_ptr<MYSQL> ret;
        database::mariadb::MariaDBConnectionDetails details;
        const std::function<void()> onConnect;
    };
} // namespace database::mariadb::command

#endif // MARIA_DB_CONNECT_COMMAND_H
