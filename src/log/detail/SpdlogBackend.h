/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef LOGGER_DETAIL_SPDLOGBACKEND_H
#define LOGGER_DETAIL_SPDLOGBACKEND_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <memory>
#include <spdlog/sinks/sink.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace spdlog {
    class logger;
} // namespace spdlog

namespace logger::detail {

    class SpdlogBackend {
    public:
        void init();

        void setQuiet(bool quiet);
        void setDisableColor(bool disableColor);
        bool getDisableColor() const;

        void setTickResolver(Logger::TickResolver resolver);

        void setLogFile(const std::string& logFile);
        void disableLogFile();

        void emitLegacy(Level level, std::string message, bool withErrno, int errnoValue);
        void emitSemantic(LogLevel level, const std::string& formattedRecord);

        bool shouldLog(Level level) const;
        bool shouldVerbose(int verboseLevel) const;

        void setLogLevel(int level);
        void setVerboseLevel(int level);
        std::string resolveTick() const;

    private:
        std::shared_ptr<spdlog::sinks::sink> stdoutSink;
        std::shared_ptr<spdlog::sinks::sink> semanticStdoutSink;
        std::shared_ptr<spdlog::sinks::sink> fileSink;
        std::shared_ptr<spdlog::sinks::sink> semanticFileSink;
        std::shared_ptr<spdlog::logger> stdoutLogger;
        std::shared_ptr<spdlog::logger> fileLogger;
        std::shared_ptr<spdlog::logger> semanticStdoutLogger;
        std::shared_ptr<spdlog::logger> semanticFileLogger;

        Logger::TickResolver tickResolver;
        int configuredLogLevel = 0;
        int configuredVerboseLevel = 0;
        bool quietMode = false;
        bool disableColor = false;
    };

} // namespace logger::detail

#endif // LOGGER_DETAIL_SPDLOGBACKEND_H
