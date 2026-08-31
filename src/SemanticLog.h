#ifndef SNODEC_SEMANTICLOG_H
#define SNODEC_SEMANTICLOG_H

#include "log/Logger.h"

#include <cerrno>

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

        inline logger::Level backendLevel(logger::LogLevel level) {
            switch (level) {
                case logger::LogLevel::Trace:
                    return logger::Level::TRACE;
                case logger::LogLevel::Debug:
                    return logger::Level::DEBUG;
                case logger::LogLevel::Info:
                    return logger::Level::INFO;
                case logger::LogLevel::Warning:
                    return logger::Level::WARNING;
                case logger::LogLevel::Error:
                    return logger::Level::ERROR;
                case logger::LogLevel::Critical:
                    return logger::Level::FATAL;
            }

            return logger::Level::INFO;
        }

    } // namespace detail

    class AppLog {
    public:
        logger::LogMessage trace() const {
            return logger::LogMessage(logger::Level::TRACE);
        }

        logger::LogMessage debug() const {
            return logger::LogMessage(logger::Level::DEBUG);
        }

        logger::LogMessage info() const {
            return logger::LogMessage(logger::Level::INFO);
        }

        logger::LogMessage warn() const {
            return logger::LogMessage(logger::Level::WARNING);
        }

        logger::LogMessage error() const {
            return logger::LogMessage(logger::Level::ERROR);
        }

        logger::LogMessage critical() const {
            return logger::LogMessage(logger::Level::FATAL);
        }
    };

    inline const AppLog& appLog() {
        static const AppLog log;
        return log;
    }

    inline logger::LogMessage sysError(const AppLog&, logger::LogLevel level, int errnum) {
        errno = errnum;
        return logger::LogMessage(detail::backendLevel(level), -1, true);
    }

} // namespace snode::semantic

#endif // SNODEC_SEMANTICLOG_H
