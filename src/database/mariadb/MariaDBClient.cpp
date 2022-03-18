#include "MariaDBClient.h"

#include "log/Logger.h"

using namespace std;

namespace database::mariadb {
    MariaDBClient::MariaDBClient(MariaDBConnectionDetails details)
        : details{details}
        , executor{make_unique<MariaDBExecutor>()} {
    }

    MariaDBClient::~MariaDBClient() {
    }

    void MariaDBClient::connect(const function<void()>& onConnect) {
        executor->connect(details, onConnect);
    }

    void MariaDBClient::disconnect(const function<void()>& onDisconnect) {
        onDisconnect();
        VLOG(0) << "Disconnect not implemented yet.";
    }
    void MariaDBClient::query(std::string sql, const std::function<void()>& onResult) {
        executor->query(sql, onResult);
    }
} // namespace database::mariadb
