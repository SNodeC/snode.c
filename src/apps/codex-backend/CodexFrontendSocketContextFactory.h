/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CODEXFRONTENDSOCKETCONTEXTFACTORY_H
#define APPS_CODEX_BACKEND_CODEXFRONTENDSOCKETCONTEXTFACTORY_H

#include "ai/openai/codex/frontend/BackendAdapter.h"
#include "apps/codex-backend/Configuration.h"
#include "core/socket/stream/SocketContextFactory.h"

namespace ai::openai::codex::backend {
    class BackendCore;
}

namespace apps::codex_backend {

    class CodexFrontendSocketContextFactory final : public core::socket::stream::SocketContextFactory {
    public:
        explicit CodexFrontendSocketContextFactory(ai::openai::codex::backend::BackendCore& backend, SocketFrontendOptions options = {});

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override;

    private:
        ai::openai::codex::frontend::BackendAdapter adapter;
        SocketFrontendOptions options;
    };

} // namespace apps::codex_backend

#endif // APPS_CODEX_BACKEND_CODEXFRONTENDSOCKETCONTEXTFACTORY_H
