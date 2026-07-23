/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_BACKEND_DETAIL_PRESERVEUNMODELEDTYPEDEVENT_H
#define AI_OPENAI_CODEX_BACKEND_DETAIL_PRESERVEUNMODELEDTYPEDEVENT_H

#include "ai/openai/codex/backend/BackendEvent.h"

#include <optional>
#include <string>

namespace ai::openai::codex::backend::detail {

    struct UnmodeledTypedEvent {
        std::string surface;
        Json rawPayload = nullptr;
        std::optional<std::string> decodingError;
        std::optional<typed::DecodeDiagnostic> diagnostic = std::nullopt;
    };

    CodexExtensionReceived preserveUnmodeledTypedEvent(UnmodeledTypedEvent event);

} // namespace ai::openai::codex::backend::detail

#endif // AI_OPENAI_CODEX_BACKEND_DETAIL_PRESERVEUNMODELEDTYPEDEVENT_H
