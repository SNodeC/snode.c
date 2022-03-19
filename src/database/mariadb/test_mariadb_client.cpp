#include "core/SNodeC.h"
#include "database/mariadb/MariaDBClient.h"
#include "log/Logger.h"

#include <chrono>
#include <mysql.h>
#include <thread>

using namespace std;
using namespace core;
using namespace database::mariadb;

void stopSNodeC(unsigned short timeout) {
    VLOG(0) << "Waiting for " << timeout / 1000 << " seconds";
    this_thread::sleep_for(chrono::milliseconds(timeout));
    VLOG(0) << "Stopping SNodeC...";
    SNodeC::free();
}

int main(int argc, char* argv[]) {
    SNodeC::init(argc, argv);
    database::mariadb::MariaDBConnectionDetails details = {
        .hostname = "localhost",
        .username = "amarok",
        .password = "pentium5",
        .database = "amarok",
        .port = 3306,
        .socket = "/run/mysqld/mysqld.sock",
        .flags = 0,
    };
    MariaDBClient db{details};
    db.connect([&]() -> void {
        VLOG(0) << "CONNECTED CALLBACK IN MAIN";
        db.query("select * from admin", []() -> void {
            VLOG(0) << "RESULT CALLBACK IN MAIN";
        });
    });
    // db.query("select * from person");
    // thread stopper_thread{stopSNodeC, 5000};
    int snodec_status{SNodeC::start()};
    // stopper_thread.join();
    return snodec_status;
}
