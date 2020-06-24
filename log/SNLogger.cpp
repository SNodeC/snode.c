#include "SNLogger.h"

#define ELPP_SYSLOG

#include <easylogging++.cc>


INITIALIZE_EASYLOGGINGPP

class SNLogger : public LoggingFacility {
public:
    virtual void writeInfoEntry(std::string_view entry) override {
        SYSLOG(INFO) << entry;
    }
    virtual void writeWarnEntry(std::string_view entry) override {
        SYSLOG(WARNING) << entry;
    }
    virtual void writeErrorEntry(std::string_view entry) override {
        SYSLOG(ERROR) << entry;
    }
};

static Logger init() {
    ELPP_INITIALIZE_SYSLOG("my_proc", LOG_PID | LOG_CONS | LOG_PERROR,
                           LOG_USER); // This is optional, you may not add it if you dont want to specify options

    return std::make_shared<SNLogger>();
}


Logger logger = init();
