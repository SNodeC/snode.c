/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/StdinReader.h"

#include "core/EventReceiver.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace apps::codex_backend_client {

    namespace {
        constexpr std::size_t MaximumBufferedLineBytes = 1024 * 1024;
    }

    StdinReader::StdinReader(LineHandler onLine, EofHandler onEof, ErrorHandler onError, int fd)
        : core::eventreceiver::ReadEventReceiver("codex-backend-client stdin", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , onLine(std::move(onLine))
        , onEof(std::move(onEof))
        , onError(std::move(onError))
        , inputFd(fd) {
        originalFlags = ::fcntl(inputFd, F_GETFL);
        if (originalFlags < 0) {
            throw std::system_error(errno, std::generic_category(), "failed to inspect stdin flags");
        }
        if ((originalFlags & O_NONBLOCK) == 0) {
            if (::fcntl(inputFd, F_SETFL, originalFlags | O_NONBLOCK) < 0) {
                throw std::system_error(errno, std::generic_category(), "failed to make stdin nonblocking");
            }
            restoreOriginalFlags = true;
        }

        const auto deferRead = [this]() {
            activeReader = true;
            deferredReader = std::make_shared<StdinReader*>(this);
            core::EventReceiver::atNextTick([weakReader = std::weak_ptr<StdinReader*>(deferredReader)]() {
                if (const std::shared_ptr<StdinReader*> reader = weakReader.lock();
                    reader && *reader != nullptr && (*reader)->activeReader) {
                    (*reader)->readEvent();
                }
            });
        };

        struct stat descriptorStatus{};
        if (::fstat(inputFd, &descriptorStatus) == 0 && S_ISREG(descriptorStatus.st_mode)) {
            // O_NONBLOCK has no effect on regular files. Refuse this input
            // source instead of performing a potentially blocking read on the
            // event-loop thread; callers can pipe the same contents instead.
            restoreFlags();
            throw std::runtime_error("regular-file stdin is unsupported; pipe commands to codex-backend-client instead");
        }

        if (S_ISCHR(descriptorStatus.st_mode) && ::isatty(inputFd) == 0) {
            // epoll rejects devices such as /dev/null. With O_NONBLOCK set,
            // one deferred drain safely observes EOF or immediately available
            // device input without registering an event receiver.
            deferRead();
            return;
        }

        if (enable(inputFd)) {
            registeredReader = true;
            activeReader = true;
            return;
        }

        restoreFlags();
        throw std::runtime_error("failed to register stdin with the SNode.C event loop");
    }

    StdinReader::~StdinReader() {
        stop();
    }

    bool StdinReader::active() const noexcept {
        return activeReader;
    }

    void StdinReader::stop() noexcept {
        activeReader = false;
        invalidateDeferredRead();
        if (registeredReader) {
            registeredReader = false;
            try {
                disable();
            } catch (...) {
            }
        }
        restoreFlags();
    }

    void StdinReader::readEvent() {
        std::array<char, 4096> bytes{};
        while (activeReader) {
            const ssize_t count = ::read(inputFd, bytes.data(), bytes.size());
            if (count > 0) {
                const std::size_t size = static_cast<std::size_t>(count);
                if (size > MaximumBufferedLineBytes || buffered.size() > MaximumBufferedLineBytes - size) {
                    fail("stdin command exceeds the one MiB line limit");
                    return;
                }
                buffered.append(bytes.data(), size);
                emitLines();
                continue;
            }
            if (count == 0) {
                finishEof();
                return;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            const int errorNumber = errno;
            fail(std::string("stdin read failed: ") + std::strerror(errorNumber));
            return;
        }
    }

    void StdinReader::unobservedEvent() {
        registeredReader = false;
        activeReader = false;
        invalidateDeferredRead();
        restoreFlags();
    }

    void StdinReader::emitLines() {
        while (activeReader) {
            const std::size_t newline = buffered.find('\n');
            if (newline == std::string::npos) {
                return;
            }
            std::string line = buffered.substr(0, newline);
            buffered.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            try {
                if (onLine) {
                    onLine(std::move(line));
                }
            } catch (...) {
                fail("stdin command callback failed");
                return;
            }
        }
    }

    void StdinReader::finishEof() noexcept {
        if (!activeReader) {
            return;
        }
        if (!buffered.empty()) {
            std::string line = std::move(buffered);
            buffered.clear();
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            try {
                if (onLine) {
                    onLine(std::move(line));
                }
            } catch (...) {
                fail("stdin command callback failed");
                return;
            }
        }
        if (!activeReader) {
            return;
        }
        stop();
        try {
            if (onEof) {
                onEof();
            }
        } catch (...) {
        }
    }

    void StdinReader::fail(std::string message) noexcept {
        if (!activeReader) {
            return;
        }
        stop();
        try {
            if (onError) {
                onError(std::move(message));
            }
        } catch (...) {
        }
    }

    void StdinReader::restoreFlags() noexcept {
        if (restoreOriginalFlags) {
            static_cast<void>(::fcntl(inputFd, F_SETFL, originalFlags));
            restoreOriginalFlags = false;
        }
    }

    void StdinReader::invalidateDeferredRead() noexcept {
        if (deferredReader) {
            *deferredReader = nullptr;
            deferredReader.reset();
        }
    }

} // namespace apps::codex_backend_client
