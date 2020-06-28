#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define ELPP_SYSLOG

#include <easylogging++.cc>
#include <iostream>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"


INITIALIZE_EASYLOGGINGPP

void Logger::init(int argc, char* argv[]) {
    START_EASYLOGGINGPP(argc, argv);

    std::string homedir;

    /*
    if ((homedir = getenv("HOME")).empty()) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    std::string logFile = "/.config/snodelogger.conf";
    el::Configurations confFromFile(homedir + logFile);
    */

    el::Configurations confFromFile("/home/voc/projects/ServerVoc/logger.conf");
    el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
    el::Loggers::reconfigureAllLoggers(confFromFile);

    setLevel(2);
}


void Logger::setLevel(int level) {
    el::Configurations defaultConf;
    defaultConf.setGlobally(el::ConfigurationType::Enabled, "false");

    bool verboseEnabled = el::Loggers::getLogger("default")->typedConfigurations()->enabled(el::Level::Verbose);
    defaultConf.set(el::Level::Verbose, el::ConfigurationType::Enabled, verboseEnabled ? "true" : "false");

    switch (level) {
    case 6:
        defaultConf.set(el::Level::Trace, el::ConfigurationType::Enabled, "true");
    case 5:
        defaultConf.set(el::Level::Debug, el::ConfigurationType::Enabled, "true");
    case 4:
        defaultConf.set(el::Level::Info, el::ConfigurationType::Enabled, "true");
    case 3:
        defaultConf.set(el::Level::Warning, el::ConfigurationType::Enabled, "true");
    case 2:
        defaultConf.set(el::Level::Error, el::ConfigurationType::Enabled, "true");
    case 1:
        defaultConf.set(el::Level::Fatal, el::ConfigurationType::Enabled, "true");
    case 0:
    default:;
    };

    el::Loggers::reconfigureLogger("default", defaultConf);
}


void Logger::logToFile(bool yes) {
    el::Configurations defaultConf;

    bool verboseEnabled = el::Loggers::getLogger("default")->typedConfigurations()->enabled(el::Level::Verbose);
    defaultConf.set(el::Level::Verbose, el::ConfigurationType::Enabled, verboseEnabled ? "true" : "false");

    if (yes) {
        defaultConf.set(el::Level::Global, el::ConfigurationType::ToFile, "true");
    } else {
        defaultConf.set(el::Level::Global, el::ConfigurationType::ToFile, "false");
    }

    el::Loggers::reconfigureLogger("default", defaultConf);
}


void Logger::logToStdOut(bool yes) {
    el::Configurations defaultConf;

    bool verboseEnabled = el::Loggers::getLogger("default")->typedConfigurations()->enabled(el::Level::Verbose);
    defaultConf.set(el::Level::Verbose, el::ConfigurationType::Enabled, verboseEnabled ? "true" : "false");

    if (yes) {
        defaultConf.set(el::Level::Global, el::ConfigurationType::ToStandardOutput, "true");
    } else {
        defaultConf.set(el::Level::Global, el::ConfigurationType::ToStandardOutput, "false");
    }

    el::Loggers::reconfigureLogger("default", defaultConf);
}
