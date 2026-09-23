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

#include "core/SNodeC.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
    std::filesystem::path tempLogPath() {
        const auto path = std::filesystem::temp_directory_path() / "snodec-async-log-drain.log";
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".1", error);
        return path;
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream in(path);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    std::size_t countOccurrences(const std::string& text, const std::string& needle) {
        std::size_t count = 0;
        std::size_t position = 0;
        while ((position = text.find(needle, position)) != std::string::npos) {
            ++count;
            position += needle.size();
        }
        return count;
    }

    logger::LogRecord record(std::string message) {
        return logger::materialize({logger::LogOrigin::Framework, logger::LogBoundary::System, "core.log-drain"},
                                   logger::LogLevel::Info,
                                   std::move(message),
                                   {});
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    if (geteuid() == 0) {
        std::cout << "SKIP: SNode.C async-log drain test requires non-root initialization" << std::endl;
        return testResult.processResult();
    }

    char application[] = "SNodeCAsyncLogDrainTest";
    char logLevel[] = "--log-level=6";
    char quiet[] = "--quiet";
    char* argv[]{application, logLevel, quiet, nullptr};
    core::SNodeC::init(3, argv);

    const auto logPath = tempLogPath();
    logger::Logger::setQuiet(true);
    logger::Logger::logToFile(logPath.string());
    logger::Logger::startAsync();

    constexpr std::size_t recordCount = 4096;
    for (std::size_t index = 0; index < recordCount; ++index) {
        logger::Logger::emitSemantic(record("async-drain-record-" + std::to_string(index)));
    }

    core::SNodeC::free();

    const std::string log = readFile(logPath);
    testResult.expectEqual(recordCount,
                           countOccurrences(log, "async-drain-record-"),
                           "SNodeC::free drains every accepted asynchronous log record before returning");
    testResult.expectTrue(log.find("async-drain-record-4095") != std::string::npos,
                          "SNodeC::free writes the last queued asynchronous log record before returning");

    logger::Logger::disableLogToFile();
    logger::Logger::init();

    return testResult.processResult();
}
