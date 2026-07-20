/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend/CodexFrontendSocketContextFactory.h"

#include "apps/codex-backend/CodexFrontendSocketContext.h"

namespace apps::codex_backend {

    CodexFrontendSocketContextFactory::CodexFrontendSocketContextFactory(ai::openai::codex::backend::BackendCore& backend,
                                                                         SocketFrontendOptions options)
        : adapter(backend)
        , options(options) {
    }

    core::socket::stream::SocketContext*
    CodexFrontendSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new CodexFrontendSocketContext(socketConnection, adapter, options);
    }

} // namespace apps::codex_backend
