/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_JSONLINEFRAMER_H
#define APPS_CODEX_BACKEND_JSONLINEFRAMER_H

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace apps::codex_backend {

    inline constexpr std::size_t DEFAULT_MAXIMUM_FRAME_SIZE = 1024 * 1024;

    class JsonLineFramer {
    public:
        enum class Result { Accepted, FrameTooLarge };
        using FrameHandler = std::function<void(std::string)>;

        explicit JsonLineFramer(std::size_t maximumFrameSize = DEFAULT_MAXIMUM_FRAME_SIZE);

        Result push(std::string_view bytes, const FrameHandler& onFrame);
        void clear() noexcept;

        std::size_t bufferedSize() const noexcept;
        std::size_t maximumFrameSize() const noexcept;

    private:
        std::size_t maximumSize;
        std::string buffered;
    };

} // namespace apps::codex_backend

#endif // APPS_CODEX_BACKEND_JSONLINEFRAMER_H
