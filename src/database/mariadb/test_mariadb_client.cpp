#include "database/mariadb/MariaDBClient.h"
#include "log/Logger.h"
#include "net/SNodeC.h"

#include <chrono>
#include <mysql.h>
#include <thread>

using namespace std;
using namespace net;
using namespace database::mariadb;

void stopSNodeC(unsigned short timeout) {
    VLOG(0) << "Waiting for " << timeout / 1000 << " seconds";
    this_thread::sleep_for(chrono::milliseconds(timeout));
    VLOG(0) << "Stopping SNodeC...";
    SNodeC::stop();
}

int main(int argc, char* argv[]) {
    SNodeC::init(argc, argv);
    MariaDBClient db = MariaDBClient();
    db.query("select * from person");
    thread stopper_thread{stopSNodeC, 5000};
    int snodec_status{SNodeC::start()};
    stopper_thread.join();
    return snodec_status;
}
