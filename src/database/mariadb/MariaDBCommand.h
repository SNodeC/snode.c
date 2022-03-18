#ifndef MARIA_DB_COMMAND
#define MARIA_DB_COMMAND

#include <memory>
#include <mysql.h>

namespace database::mariadb {
    class MariaDBCommand {
    public:
        MariaDBCommand(std::shared_ptr<MYSQL> mysql);
        virtual int start() = 0;
        virtual int cont(int&) = 0;
        virtual void onComplete() = 0;

    protected:
        std::shared_ptr<MYSQL> mysql;
    };
} // namespace database::mariadb

#endif // MARIA_DB_COMMAND
