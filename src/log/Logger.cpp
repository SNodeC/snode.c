/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

INITIALIZE_EASYLOGGINGPP

namespace logger {

    el::Configurations Logger::conf;

    void Logger::init() {
        conf.setGlobally(el::ConfigurationType::Enabled, "true");
        conf.setGlobally(el::ConfigurationType::Format, "%datetime{%Y-%M-%d %H:%m:%s} %tick: %level %msg");
        conf.setGlobally(el::ConfigurationType::ToFile, "false");
        conf.setGlobally(el::ConfigurationType::ToStandardOutput, "true");
        conf.setGlobally(el::ConfigurationType::SubsecondPrecision, "2");
        conf.setGlobally(el::ConfigurationType::PerformanceTracking, "false");
        conf.setGlobally(el::ConfigurationType::MaxLogFileSize, "2097152");
        conf.setGlobally(el::ConfigurationType::LogFlushThreshold, "0");
        conf.set(el::Level::Verbose, el::ConfigurationType::Format, "%datetime{%Y-%M-%d %H:%m:%s} %tick: %msg");

        el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
        el::Loggers::addFlag(el::LoggingFlag::DisablePerformanceTrackingCheckpointComparison);
        el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

        el::Loggers::removeFlag(el::LoggingFlag::AllowVerboseIfModuleNotSpecified);

        el::Loggers::reconfigureLogger("default", conf);

        el::Logger* logger = el::Loggers::getLogger("default");
        std::string logFilename = logger->typedConfigurations()->filename(el::Level::Verbose).c_str();

        if (logFilename != "/dev/null") {
            conf.setGlobally(el::ConfigurationType::ToFile, "true");
            el::Loggers::reconfigureLogger("default", conf);
        }
    }

    void Logger::setCustomFormatSpec(const char* format, const el::FormatSpecifierValueResolver& resolver) {
        el::Helpers::installCustomFormatSpecifier(el::CustomFormatSpecifier(format, resolver));
    }

    // Application logging should be done with VLOG(loglevel)
    // Framework logging should use one of the following levels
    void Logger::setLogLevel(int level) {
        conf.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");   // trace method/function calling
        conf.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");   // typical assert messages - but we can go on
        conf.set(el::Level::Info, el::ConfigurationType::Enabled, "false");    // additional infos - what 's going on
        conf.set(el::Level::Warning, el::ConfigurationType::Enabled, "false"); // not that serious but mentioning
        conf.set(el::Level::Error, el::ConfigurationType::Enabled, "false");   // serious errors - but we can go on
        conf.set(el::Level::Fatal, el::ConfigurationType::Enabled, "false");   // asserts - stop - we can not go on

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
                [[fallthrough]];
            default:;
        }

        el::Loggers::reconfigureLogger("default", conf);
    }

    void Logger::setVerboseLevel(int level) {
        if (level >= 0) {
            el::Loggers::setVerboseLevel(static_cast<el::base::type::VerboseLevel>(level));
        }
    }

    void Logger::logToFile(const std::string& logFile) {
        conf.setGlobally(el::ConfigurationType::Filename, logFile);
        conf.setGlobally(el::ConfigurationType::ToFile, "true");

        el::Loggers::reconfigureLogger("default", conf);
    }

    void Logger::quiet() {
        conf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");

        el::Loggers::reconfigureLogger("default", conf);
    }

} // namespace logger
