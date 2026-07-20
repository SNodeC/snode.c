/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CLIENT_STDINREADER_H
#define APPS_CODEX_BACKEND_CLIENT_STDINREADER_H

#include "core/eventreceiver/ReadEventReceiver.h"

#include <functional>
#include <memory>
#include <string>

namespace apps::codex_backend_client {

    class StdinReader final : private core::eventreceiver::ReadEventReceiver {
    public:
        using LineHandler = std::function<void(std::string)>;
        using EofHandler = std::function<void()>;
        using ErrorHandler = std::function<void(std::string)>;

        explicit StdinReader(LineHandler onLine, EofHandler onEof, ErrorHandler onError = {}, int fd = 0);
        StdinReader(const StdinReader&) = delete;
        StdinReader(StdinReader&&) = delete;

        StdinReader& operator=(const StdinReader&) = delete;
        StdinReader& operator=(StdinReader&&) = delete;

        ~StdinReader() override;

        [[nodiscard]] bool active() const noexcept;
        void stop() noexcept;

    private:
        void readEvent() override;
        void unobservedEvent() override;

        void emitLines();
        void finishEof() noexcept;
        void fail(std::string message) noexcept;
        void invalidateDeferredRead() noexcept;
        void restoreFlags() noexcept;

        LineHandler onLine;
        EofHandler onEof;
        ErrorHandler onError;
        int inputFd;
        int originalFlags = -1;
        bool restoreOriginalFlags = false;
        bool activeReader = false;
        bool registeredReader = false;
        std::shared_ptr<StdinReader*> deferredReader;
        std::string buffered;
    };

} // namespace apps::codex_backend_client

#endif // APPS_CODEX_BACKEND_CLIENT_STDINREADER_H
