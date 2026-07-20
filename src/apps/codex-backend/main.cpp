/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/backend/BackendCore.h"
#include "ai/openai/codex/stdio/Client.h"
#include "apps/codex-backend/CodexFrontendSocketContextFactory.h"
#include "apps/codex-backend/Configuration.h"
#include "core/SNodeC.h"
#include "net/un/stream/legacy/SocketServer.h"

#include <iostream>
#include <memory>
#include <utility>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    int result = 1;
    {
        auto appServer = std::make_unique<ai::openai::codex::stdio::Client>();
        ai::openai::codex::backend::BackendCore backend(std::move(appServer));

        auto socketServer = net::un::stream::legacy::Server<apps::codex_backend::CodexFrontendSocketContextFactory>(
            "codex-backend",
            [](net::un::stream::legacy::config::ConfigSocketServer* config) {
                // setSunPath supplies the safe application default while keeping
                // the ordinary SNode.C --sun-path/config-file option authoritative.
                config->Local::setSunPath(apps::codex_backend::defaultSocketPath());
            },
            backend);
        socketServer.listen([&backend](const net::un::SocketAddress& address, core::socket::State state) {
            if (state != core::socket::State::OK) {
                std::cerr << "codex-backend: failed to listen on Unix socket " << address.toString() << '\n';
                backend.stop();
                core::SNodeC::stop();
            }
        });

        backend.start();
        result = core::SNodeC::start();
    }

    // EventLoop::start performs the primary global shutdown. free() is
    // intentionally repeated after the socket server and backend have been
    // destroyed; the SNode.C lifecycle makes this cleanup idempotent.
    core::SNodeC::free();
    return result;
}
