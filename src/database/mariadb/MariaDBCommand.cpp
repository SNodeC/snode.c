#include "database/mariadb/MariaDBCommand.h"

namespace database::mariadb {
    MariaDBCommand::MariaDBCommand(const std::function<void()>& onComplete)
        : onComplete{onComplete} {
    }
} // namespace database::mariadb
