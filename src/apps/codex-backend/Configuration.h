/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CONFIGURATION_H
#define APPS_CODEX_BACKEND_CONFIGURATION_H

#include "ai/openai/codex/frontend/Protocol.h"
#include "apps/codex-backend/JsonLineFramer.h"

#include <cstddef>
#include <string>

namespace apps::codex_backend {

    // Keep transport framing and already-delivered writer data outside the
    // reusable adapter's bounded queue. This remains finite while ensuring a
    // freshly connected Unix client can accept a maximum-sized default replay.
    inline constexpr std::size_t DEFAULT_MAXIMUM_OUTBOUND_BYTES = 13 * 1024 * 1024;

    static_assert(DEFAULT_MAXIMUM_OUTBOUND_BYTES >= ai::openai::codex::frontend::DefaultAdapterMaxOutboundBytes +
                                                        ai::openai::codex::frontend::DefaultAdapterMaxOutboundMessages);

    struct SocketFrontendOptions {
        std::size_t maximumFrameSize = DEFAULT_MAXIMUM_FRAME_SIZE;
        std::size_t maximumOutboundBytes = DEFAULT_MAXIMUM_OUTBOUND_BYTES;
    };

    std::string defaultSocketPath();

} // namespace apps::codex_backend

#endif // APPS_CODEX_BACKEND_CONFIGURATION_H
