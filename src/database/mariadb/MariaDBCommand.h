#ifndef MARIA_DB_COMMAND
#define MARIA_DB_COMMAND

#include <functional>

namespace database::mariadb {
    class MariaDBCommand {
    public:
        MariaDBCommand(const std::function<void()>& onComplete);
        const std::function<void()> onComplete;
        virtual int start() = 0;
        virtual int cont(int&) = 0;
    };
} // namespace database::mariadb

#endif // MARIA_DB_COMMAND
