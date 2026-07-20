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
#include <fcntl.h>
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

    class LineReader {
    public:
        bool readLine(std::string& line, bool slow = false) {
            line.clear();
            char chunk[1024]{};

            while (true) {
                const std::size_t newline = buffer.find('\n');
                if (newline != std::string::npos) {
                    line.assign(buffer, 0, newline);
                    buffer.erase(0, newline + 1);
                    return true;
                }

                const ssize_t count = ::read(STDIN_FILENO, chunk, sizeof(chunk));
                if (count > 0) {
                    buffer.append(chunk, static_cast<std::size_t>(count));
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

        bool waitForEof(bool rejectAdditionalData = false) {
            if (rejectAdditionalData && !buffer.empty()) {
                return false;
            }
            buffer.clear();

            char chunk[1024]{};
            while (true) {
                const ssize_t count = ::read(STDIN_FILENO, chunk, sizeof(chunk));
                if (count == 0) {
                    return true;
                }
                if (count > 0) {
                    if (rejectAdditionalData) {
                        return false;
                    }
                } else if (errno != EINTR) {
                    return false;
                }
            }
        }

    private:
        std::string buffer;
    };

    bool isBlockingDescriptor(int fd) {
        const int flags = ::fcntl(fd, F_GETFL);
        return flags >= 0 && (flags & O_NONBLOCK) == 0;
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

    bool receiveInitialized(LineReader& input) {
        std::string initialized;
        return input.readLine(initialized) && !initialized.empty() && initialized.find("\"method\":\"initialized\"") != std::string::npos;
    }
} // namespace

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "normal";

    if (mode.starts_with("pidfile-")) {
        if (argc < 3) {
            return 30;
        }
        const int pidFile = ::open(argv[2], O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        if (pidFile < 0 || !writeAll(pidFile, std::to_string(::getpid())) || ::close(pidFile) != 0) {
            return 31;
        }
        mode.erase(0, std::string("pidfile-").size());
    }

    if (mode == "exit-before-initialize") {
        return 17;
    }

    if (!isBlockingDescriptor(STDIN_FILENO) || !isBlockingDescriptor(STDOUT_FILENO) || !isBlockingDescriptor(STDERR_FILENO)) {
        return 18;
    }

    if (mode == "never-read-stdin" || mode == "never-read-stdin-ignore-term") {
        if (mode == "never-read-stdin-ignore-term") {
            std::signal(SIGTERM, SIG_IGN);
        }
        while (true) {
            ::pause();
        }
    }

    LineReader input;
    std::string initialize;
    if (!input.readLine(initialize, mode == "slow-stdin") || initialize.empty() ||
        initialize.find("\"method\":\"initialize\"") == std::string::npos) {
        return 20;
    }

    if (mode == "close-stdout") {
        ::close(STDOUT_FILENO);
        static_cast<void>(input.waitForEof());
        return 0;
    }

    if (mode == "hold-initialize") {
        static_cast<void>(input.waitForEof());
        return 0;
    }

    bool rejectInitialization = mode == "initialization-error";
    if (mode == "fail-once") {
        if (argc < 3) {
            return 24;
        }
        const int marker = ::open(argv[2], O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (marker >= 0) {
            ::close(marker);
            rejectInitialization = true;
        } else if (errno != EEXIST) {
            return 25;
        }
    }

    if (rejectInitialization) {
        const std::string response = "{\"id\":" + std::to_string(requestId(initialize)) +
                                     ",\"error\":{\"code\":-32000,\"message\":\"fake initialization rejected\"}}\n";
        writeAll(STDOUT_FILENO, response);
        static_cast<void>(input.waitForEof());
        return 0;
    }

    if (mode == "malformed-json") {
        writeAll(STDOUT_FILENO, "{not-json}\n");
        static_cast<void>(input.waitForEof());
        return 0;
    }

    if (!writeAll(STDERR_FILENO, "diagnostic one\ndiagnostic") || !writeAll(STDERR_FILENO, " two\n") ||
        !writeAll(STDERR_FILENO, "{\"id\":0,\"error\":{\"code\":99,\"message\":\"stderr only\"}}\n") ||
        !sendNormalHandshakeResponse(requestId(initialize)) || !receiveInitialized(input)) {
        return 21;
    }

    if (mode == "close-stderr") {
        if (!writeAll(STDERR_FILENO, "trailing diagnostic without newline")) {
            return 22;
        }
        ::close(STDERR_FILENO);
        if (!writeAll(STDOUT_FILENO, "{\"method\":\"fake/afterStderrClose\",\"params\":{}}\n")) {
            return 23;
        }
        return input.waitForEof() ? 0 : 26;
    }

    if (mode == "stdio-framing") {
        if (!input.waitForEof(true)) {
            return 27;
        }
        return writeAll(STDERR_FILENO, "stdio-framing-ok\n") ? 0 : 28;
    }

    if (!writeAll(STDERR_FILENO, mode == "slow-stdin" ? "slow-initialized\n" : "initialized-ok\n")) {
        return 22;
    }

    if (mode == "ignore-shutdown") {
        std::signal(SIGTERM, SIG_IGN);
        static_cast<void>(input.waitForEof());
        while (true) {
            ::pause();
        }
    }

    return input.waitForEof() ? 0 : 29;
}
