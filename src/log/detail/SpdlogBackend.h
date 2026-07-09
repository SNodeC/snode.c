/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#ifndef LOGGER_DETAIL_SPDLOGBACKEND_H
#define LOGGER_DETAIL_SPDLOGBACKEND_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace logger {

    enum class Level;
    enum class LogLevel;

    namespace detail {

        class SpdlogBackend {
        public:
            using TickResolver = std::function<std::string()>;

            SpdlogBackend();
            ~SpdlogBackend();

            SpdlogBackend(const SpdlogBackend&) = delete;
            SpdlogBackend& operator=(const SpdlogBackend&) = delete;
            SpdlogBackend(SpdlogBackend&&) = delete;
            SpdlogBackend& operator=(SpdlogBackend&&) = delete;

            void init();
            void setTickResolver(TickResolver resolver);
            void setLogLevel(int level);
            void setVerboseLevel(int level);
            void setQuiet(bool quiet);
            void setDisableColor(bool disableColor);
            bool getDisableColor() const;

            void setLogFile(const std::string& logFile);
            void disableLogFile();

            bool shouldLog(Level level) const;
            bool shouldVerbose(int verboseLevel) const;

            void emitLegacy(Level level, std::string message, bool withErrno, int errnoValue);
            void emitSemantic(LogLevel level, const std::string& formattedRecord);

        private:
            class Impl;

            std::unique_ptr<Impl> impl;
        };

    } // namespace detail
} // namespace logger

#endif // LOGGER_DETAIL_SPDLOGBACKEND_H
