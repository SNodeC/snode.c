/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_BACKEND_REDUCER_H
#define AI_OPENAI_CODEX_BACKEND_REDUCER_H

#include "ai/openai/codex/backend/BackendEvent.h"

#include <cstddef>
#include <vector>

namespace ai::openai::codex::backend {

    struct ReducerOptions {
        std::size_t retainedDiagnostics = 64;
        std::size_t retainedExtensions = 64;
        std::size_t maxDiagnosticBytes = 16U * 1024U;
        std::size_t maxExtensionMethodBytes = 4U * 1024U;
        std::size_t maxExtensionBytes = 64U * 1024U;
        std::size_t maxExtensionDecodingErrorBytes = 16U * 1024U;
        std::size_t maxAccumulatedItemBytes = 4U * 1024U * 1024U;
        std::size_t maxModelReroutesPerTurn = 64;
    };

    struct Reduction {
        bool changed = false;
        bool flushImmediately = false;
    };

    class Reducer {
    public:
        explicit Reducer(ReducerOptions options = {});

        Reduction apply(BackendState& state, const BackendEvent& event) const;
        std::vector<BackendEvent> translate(const typed::Event& event) const;

    private:
        ReducerOptions options;
    };

} // namespace ai::openai::codex::backend

#endif // AI_OPENAI_CODEX_BACKEND_REDUCER_H
