/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <string>
#include <string_view>
#include <unistd.h>

namespace {
    bool writeAll(int fd, std::string_view data) {
        std::size_t offset = 0;
        while (offset < data.size()) {
            const ssize_t written = ::write(fd, data.data() + offset, data.size() - offset);
            if (written > 0) {
                offset += static_cast<std::size_t>(written);
            } else if (written < 0 && errno == EINTR) {
                continue;
            } else {
                return false;
            }
        }
        return true;
    }

    bool readLine(std::string& line, bool slow = false) {
        line.clear();
        char buffer[1024]{};

        while (true) {
            const ssize_t count = ::read(STDIN_FILENO, buffer, sizeof(buffer));
            if (count > 0) {
                line.append(buffer, static_cast<std::size_t>(count));
                const std::size_t newline = line.find('\n');
                if (newline != std::string::npos) {
                    line.resize(newline);
                    return true;
                }
                if (slow) {
                    timespec delay{0, 1000000};
                    while (::nanosleep(&delay, &delay) != 0 && errno == EINTR) {
                    }
                }
            } else if (count == 0) {
                return false;
            } else if (errno != EINTR) {
                return false;
            }
        }
    }

    long long requestId(const std::string& request) {
        const std::size_t idField = request.find("\"id\"");
        if (idField == std::string::npos) {
            return 0;
        }
        const std::size_t colon = request.find(':', idField);
        if (colon == std::string::npos) {
            return 0;
        }
        return std::strtoll(request.c_str() + static_cast<std::ptrdiff_t>(colon + 1), nullptr, 10);
    }

    std::string initializeResponse(long long id) {
        return "{\"id\":" + std::to_string(id) +
               ",\"result\":{\"codexHome\":\"/tmp/fake-codex\",\"platformFamily\":\"unix\",\"platformOs\":\"linux\","
               "\"userAgent\":\"snodec-fake\"}}\n";
    }

    bool sendNormalHandshakeResponse(long long id) {
        const std::string response = initializeResponse(id);
        const std::size_t split = response.size() / 3;
        const std::string firstBatch = "{\"method\":\"fake/beforeInitialize\",\"params\":{}}\n"
                                       "{\"id\":999,\"result\":{\"ignored\":true}}\n" +
                                       response.substr(0, split);
        const std::string secondBatch = response.substr(split) + "{\"method\":\"fake/afterInitialize\",\"params\":{}}\n"
                                                                 "{\"method\":\"fake/secondNotification\",\"params\":{}}\n";

        return writeAll(STDOUT_FILENO, firstBatch) && writeAll(STDOUT_FILENO, secondBatch);
    }

    bool receiveInitialized() {
        std::string initialized;
        return readLine(initialized) && initialized.find("\"method\":\"initialized\"") != std::string::npos;
    }

    void waitForEof() {
        char buffer[1024]{};
        while (true) {
            const ssize_t count = ::read(STDIN_FILENO, buffer, sizeof(buffer));
            if (count == 0) {
                return;
            }
            if (count < 0 && errno != EINTR) {
                return;
            }
        }
    }
} // namespace

int main(int argc, char* argv[]) {
    const std::string mode = argc > 1 ? argv[1] : "normal";

    if (mode == "exit-before-initialize") {
        return 17;
    }

    std::string initialize;
    if (!readLine(initialize, mode == "slow-stdin")) {
        return 20;
    }

    if (mode == "close-stdout") {
        ::close(STDOUT_FILENO);
        waitForEof();
        return 0;
    }

    if (mode == "hold-initialize") {
        waitForEof();
        return 0;
    }

    if (mode == "initialization-error") {
        const std::string response = "{\"id\":" + std::to_string(requestId(initialize)) +
                                     ",\"error\":{\"code\":-32000,\"message\":\"fake initialization rejected\"}}\n";
        writeAll(STDOUT_FILENO, response);
        waitForEof();
        return 0;
    }

    if (mode == "malformed-json") {
        writeAll(STDOUT_FILENO, "{not-json}\n");
        waitForEof();
        return 0;
    }

    if (!writeAll(STDERR_FILENO, "diagnostic one\ndiagnostic") || !writeAll(STDERR_FILENO, " two\n") ||
        !writeAll(STDERR_FILENO, "{\"id\":0,\"error\":{\"code\":99,\"message\":\"stderr only\"}}\n") ||
        !sendNormalHandshakeResponse(requestId(initialize)) || !receiveInitialized()) {
        return 21;
    }

    if (!writeAll(STDERR_FILENO, mode == "slow-stdin" ? "slow-initialized\n" : "initialized-ok\n")) {
        return 22;
    }

    if (mode == "ignore-shutdown") {
        std::signal(SIGTERM, SIG_IGN);
        waitForEof();
        while (true) {
            ::pause();
        }
    }

    waitForEof();
    return 0;
}
