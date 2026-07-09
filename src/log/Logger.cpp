/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include "log/SemanticLogger.h"
#include "log/detail/SpdlogBackend.h"

#include <cerrno>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
    logger::detail::SpdlogBackend backend;
} // namespace

namespace logger {

    void Logger::init() {
        backend.init();
        disableColorLog = backend.getDisableColor();
    }

    void Logger::setTickResolver(TickResolver resolver) {
        backend.setTickResolver(std::move(resolver));
    }

    void Logger::setLogLevel(int level) {
        backend.setLogLevel(level);
    }

    void Logger::setVerboseLevel(int level) {
        backend.setVerboseLevel(level);
    }

    void Logger::logToFile(const std::string& logFile) {
        backend.setLogFile(logFile);
    }

    void Logger::disableLogToFile() {
        backend.disableLogFile();
    }

    void Logger::setQuiet(bool quiet) {
        backend.setQuiet(quiet);
    }

    void Logger::setDisableColor(bool disableColor) {
        backend.setDisableColor(disableColor);
        disableColorLog = disableColor;
    }

    bool Logger::getDisableColor() {
        return backend.getDisableColor();
    }

    bool Logger::shouldLog(Level level) {
        return backend.shouldLog(level);
    }

    bool Logger::shouldVerbose(int verboseLevel) {
        return backend.shouldVerbose(verboseLevel);
    }

    void Logger::emitSemantic(const LogRecord& record) {
        if (!LogManager::shouldEmit(record)) {
            return;
        }

        if (LogManager::format() == LogManager::Format::Json) {
            const std::string json = formatJsonV1(record);
            backend.emitSemantic(record.level, json, json);
        } else {
            backend.emitSemantic(record.level, formatText(record), formatText(record, true));
        }
    }

    BoundaryLogger::Sink Logger::semanticSink() {
        return [](LogRecord record) {
            Logger::emitSemantic(record);
        };
    }

    bool Logger::disableColorLog = false;

    LogMessage::LogMessage(const Level level, const int verboseLevel, const bool withErrno)
        : level(level)
        , verboseLevel(verboseLevel)
        , withErrno(withErrno)
        , enabled(true)
        , errnoValue(errno) {
    }

    LogMessage::~LogMessage() {
        if (enabled) {
            backend.emitLegacy(level, message.str(), withErrno, errnoValue);
        }
    }

    std::ostringstream& LogMessage::stream() {
        return message;
    }

} // namespace logger

std::ostream& Color::operator<<(std::ostream& os, const Code& code) {
    return os << (!logger::Logger::disableColorLog ? ("\033[" + std::to_string(static_cast<int>(code)) + "m") : "");
}

std::string Color::operator+(const std::string& string, const Code& code) {
    return string + (!logger::Logger::disableColorLog ? ("\033[" + std::to_string(static_cast<int>(code)) + "m") : "");
}

std::string Color::operator+(const Code& code, const std::string& string) {
    return (!logger::Logger::disableColorLog ? ("\033[" + std::to_string(static_cast<int>(code)) + "m") : "") + string;
}
