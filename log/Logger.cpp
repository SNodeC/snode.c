/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <filesystem>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"

INITIALIZE_EASYLOGGINGPP

el::Configurations Logger::conf;

void Logger::init(int argc, char* argv[]) {
    START_EASYLOGGINGPP(argc, argv);

    std::string dir = weakly_canonical(std::filesystem::path(argv[0])).parent_path();

    conf = el::Configurations(el::Configurations(dir + "/.logger.conf"));

    el::Loggers::reconfigureAllLoggers(conf);

    setLogLevel(6);
}

void Logger::setCustomFormatSpec(const char* format, const el::FormatSpecifierValueResolver& resolver) {
    el::Helpers::installCustomFormatSpecifier(el::CustomFormatSpecifier(format, resolver));
}

void Logger::setLogLevel(int level) {
    conf.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
    conf.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
    conf.set(el::Level::Info, el::ConfigurationType::Enabled, "false");
    conf.set(el::Level::Warning, el::ConfigurationType::Enabled, "false");
    conf.set(el::Level::Error, el::ConfigurationType::Enabled, "false");
    conf.set(el::Level::Fatal, el::ConfigurationType::Enabled, "false");

    switch (level) {
        case 6:
            conf.set(el::Level::Trace, el::ConfigurationType::Enabled, "true");
            [[fallthrough]];
        case 5:
            conf.set(el::Level::Debug, el::ConfigurationType::Enabled, "true");
            [[fallthrough]];
        case 4:
            conf.set(el::Level::Info, el::ConfigurationType::Enabled, "true");
            [[fallthrough]];
        case 3:
            conf.set(el::Level::Warning, el::ConfigurationType::Enabled, "true");
            [[fallthrough]];
        case 2:
            conf.set(el::Level::Error, el::ConfigurationType::Enabled, "true");
            [[fallthrough]];
        case 1:
            conf.set(el::Level::Fatal, el::ConfigurationType::Enabled, "true");
            [[fallthrough]];
        case 0:
        default:;
    };

    el::Loggers::reconfigureLogger("default", conf);
}

void Logger::setVerboseLevel(int level) {
    el::Loggers::setVerboseLevel(level);
}

void Logger::logToFile(bool yes) {
    if (yes) {
        conf.set(el::Level::Global, el::ConfigurationType::ToFile, "true");
    } else {
        conf.set(el::Level::Global, el::ConfigurationType::ToFile, "false");
    }

    el::Loggers::reconfigureLogger("default", conf);
}

void Logger::logToStdOut(bool yes) {
    if (yes) {
        conf.set(el::Level::Global, el::ConfigurationType::ToStandardOutput, "true");
    } else {
        conf.set(el::Level::Global, el::ConfigurationType::ToStandardOutput, "false");
    }

    el::Loggers::reconfigureLogger("default", conf);
}
