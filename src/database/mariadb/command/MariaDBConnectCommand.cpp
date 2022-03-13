#include "database/mariadb/command/MariaDBConnectCommand.h"

using namespace std;

namespace database::mariadb::command {
    MariaDBConnectCommand::MariaDBConnectCommand(shared_ptr<MYSQL> mysql,
                                                 MariaDBConnectionDetails details,
                                                 const function<void()>& onConnect)
        : MariaDBCommand::MariaDBCommand{onConnect}
        , ret{}
        , mysql{mysql}
        , details{details} {
    }

    int MariaDBConnectCommand::start() {
        MYSQL* retPtr = ret.get();
        return mysql_real_connect_start(&retPtr,
                                        mysql.get(),
                                        details.hostname.c_str(),
                                        details.username.c_str(),
                                        details.password.c_str(),
                                        details.database.c_str(),
                                        details.port,
                                        details.socket.c_str(),
                                        details.flags);
    }

    int MariaDBConnectCommand::cont(int& status) {
        MYSQL* retPtr = ret.get();
        return mysql_real_connect_cont(&retPtr, mysql.get(), status);
    }
} // namespace database::mariadb::command
