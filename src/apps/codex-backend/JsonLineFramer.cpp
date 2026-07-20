/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend/JsonLineFramer.h"

#include <stdexcept>
#include <utility>

namespace apps::codex_backend {

    JsonLineFramer::JsonLineFramer(std::size_t maximumFrameSize)
        : maximumSize(maximumFrameSize) {
        if (maximumSize == 0) {
            throw std::invalid_argument("the maximum JSONL frame size must be greater than zero");
        }
        buffered.reserve(maximumSize < 4096 ? maximumSize : 4096);
    }

    JsonLineFramer::Result JsonLineFramer::push(std::string_view bytes, const FrameHandler& onFrame) {
        for (const char byte : bytes) {
            if (byte == '\n') {
                if (!buffered.empty() && buffered.back() == '\r') {
                    buffered.pop_back();
                }
                std::string frame;
                frame.swap(buffered);
                onFrame(std::move(frame));
                continue;
            }

            if (buffered.size() == maximumSize) {
                clear();
                return Result::FrameTooLarge;
            }
            buffered.push_back(byte);
        }

        return Result::Accepted;
    }

    void JsonLineFramer::clear() noexcept {
        buffered.clear();
    }

    std::size_t JsonLineFramer::bufferedSize() const noexcept {
        return buffered.size();
    }

    std::size_t JsonLineFramer::maximumFrameSize() const noexcept {
        return maximumSize;
    }

} // namespace apps::codex_backend
