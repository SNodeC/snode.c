/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CLIENT_CODEXBACKENDCLIENTSOCKETCONTEXTFACTORY_H
#define APPS_CODEX_BACKEND_CLIENT_CODEXBACKENDCLIENTSOCKETCONTEXTFACTORY_H

#include "apps/codex-backend-client/JsonLineFramer.h"
#include "core/socket/stream/SocketContextFactory.h"

#include <cstddef>

namespace apps::codex_backend_client {

    class ClientConnection;

    class CodexBackendClientSocketContextFactory final : public core::socket::stream::SocketContextFactory {
    public:
        explicit CodexBackendClientSocketContextFactory(ClientConnection& connection,
                                                        std::size_t maximumFrameSize = DEFAULT_MAXIMUM_FRAME_SIZE);

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override;

    private:
        ClientConnection& connection;
        std::size_t maximumFrameSize;
    };

} // namespace apps::codex_backend_client

#endif // APPS_CODEX_BACKEND_CLIENT_CODEXBACKENDCLIENTSOCKETCONTEXTFACTORY_H
