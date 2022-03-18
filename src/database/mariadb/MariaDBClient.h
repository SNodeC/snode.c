#ifndef MARIA_DB_CLIENT
#define MARIA_DB_CLIENT

#include "MariaDBConnectionDetails.h"
#include "MariaDBExecutor.h"

#include <functional>
#include <memory>
#include <string>

namespace database::mariadb {
    class MariaDBClient {
    public:
        MariaDBClient(MariaDBConnectionDetails details);
        ~MariaDBClient();
        void connect(const std::function<void()>& onConnect);
        void disconnect(const std::function<void()>& onDisconnect);
        void query(std::string sql, const std::function<void()>& onResult);

    private:
        MariaDBConnectionDetails details;
        std::unique_ptr<MariaDBExecutor> executor;
    };
} // namespace database::mariadb

#endif // MARIA_DB_CLIENT
