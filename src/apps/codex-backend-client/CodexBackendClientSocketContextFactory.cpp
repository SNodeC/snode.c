/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/CodexBackendClientSocketContextFactory.h"

#include "apps/codex-backend-client/CodexBackendClientSocketContext.h"

namespace apps::codex_backend_client {

    CodexBackendClientSocketContextFactory::CodexBackendClientSocketContextFactory(ClientConnection& connection,
                                                                                   std::size_t maximumFrameSize)
        : connection(connection)
        , maximumFrameSize(maximumFrameSize) {
    }

    core::socket::stream::SocketContext*
    CodexBackendClientSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new CodexBackendClientSocketContext(socketConnection, connection, maximumFrameSize);
    }

} // namespace apps::codex_backend_client
