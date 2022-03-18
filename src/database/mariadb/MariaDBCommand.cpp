#include "database/mariadb/MariaDBCommand.h"

using namespace std;

namespace database::mariadb {
    MariaDBCommand::MariaDBCommand(shared_ptr<MYSQL> mysql)
        : mysql{mysql} {
    }
} // namespace database::mariadb
