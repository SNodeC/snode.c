/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef SNODEC_SEMANTICLOG_H
#define SNODEC_SEMANTICLOG_H

#include <log/Logger.h>

#include <cerrno>
#include <ostream>

namespace logger {

    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical,
    };

} // namespace logger

namespace snode::semantic {

    namespace detail {

        inline el::Level backendLevel(logger::LogLevel level) {
            switch (level) {
                case logger::LogLevel::Trace:
                    return el::Level::Trace;
                case logger::LogLevel::Debug:
                    return el::Level::Debug;
                case logger::LogLevel::Info:
                    return el::Level::Info;
                case logger::LogLevel::Warning:
                    return el::Level::Warning;
                case logger::LogLevel::Error:
                    return el::Level::Error;
                case logger::LogLevel::Critical:
                    return el::Level::Fatal;
            }

            return el::Level::Info;
        }

        inline el::Level captureSystemError(logger::LogLevel level, int errnum) {
            errno = errnum;
            return backendLevel(level);
        }

    } // namespace detail

    class LogStream {
    public:
        explicit LogStream(logger::LogLevel level)
            : writer(detail::backendLevel(level), "", 0, "") {
            writer.construct(el::Loggers::getLogger("default"));
        }

        template <typename Value>
        LogStream& operator<<(const Value& value) {
            writer << value;
            return *this;
        }

        LogStream& operator<<(std::ostream& (*manipulator)(std::ostream&)) {
            writer << manipulator;
            return *this;
        }

    private:
        el::base::Writer writer;
    };

    class SystemErrorStream {
    public:
        SystemErrorStream(logger::LogLevel level, int errnum)
            : writer(detail::captureSystemError(level, errnum), "", 0, "") {
            writer.construct(el::Loggers::getLogger("default"));
        }

        template <typename Value>
        SystemErrorStream& operator<<(const Value& value) {
            writer << value;
            return *this;
        }

        SystemErrorStream& operator<<(std::ostream& (*manipulator)(std::ostream&)) {
            writer << manipulator;
            return *this;
        }

    private:
        el::base::PErrorWriter writer;
    };

    class AppLog {
    public:
        LogStream trace() const {
            return LogStream(logger::LogLevel::Trace);
        }

        LogStream debug() const {
            return LogStream(logger::LogLevel::Debug);
        }

        LogStream info() const {
            return LogStream(logger::LogLevel::Info);
        }

        LogStream warn() const {
            return LogStream(logger::LogLevel::Warning);
        }

        LogStream error() const {
            return LogStream(logger::LogLevel::Error);
        }

        LogStream critical() const {
            return LogStream(logger::LogLevel::Critical);
        }
    };

    inline const AppLog& appLog() {
        static const AppLog log;
        return log;
    }

    inline SystemErrorStream sysError(const AppLog&, logger::LogLevel level, int errnum) {
        return SystemErrorStream(level, errnum);
    }

} // namespace snode::semantic

#endif // SNODEC_SEMANTICLOG_H
