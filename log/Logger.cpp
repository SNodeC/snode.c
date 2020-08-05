/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define ELPP_SYSLOG

#include <easylogging++.cc>
#include <easylogging++.h>
#include <string>

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
