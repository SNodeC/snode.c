#ifndef MARIA_DB_CONNECTION_DETAILS
#define MARIA_DB_CONNECTION_DETAILS

#include <string>

namespace database::mariadb {
    struct MariaDBConnectionDetails {
        std::string hostname;
        std::string username;
        std::string password;
        std::string database;
        unsigned int port;
        std::string socket;
        unsigned int flags;
    };
} // namespace database::mariadb

#endif // MARIA_DB_CONNECTION_DETAILS
