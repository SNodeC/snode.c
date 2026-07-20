/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/frontend/Messages.h"
#include "apps/codex-backend-client/ClientConnection.h"
#include "apps/codex-backend-client/CodexBackendClientSocketContextFactory.h"
#include "apps/codex-backend-client/CommandParser.h"
#include "apps/codex-backend-client/Configuration.h"
#include "apps/codex-backend-client/Presenter.h"
#include "apps/codex-backend-client/StdinReader.h"
#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "log/Logger.h"
#include "net/un/SocketAddress.h"
#include "net/un/stream/legacy/SocketClient.h"
#include "utils/Config.h"

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

int main(int argc, char* argv[]) {
    CLI::Option* const jsonOption = utils::Config::configRoot
                                        .addFlagFunction(
                                            "--json",
                                            []() {
                                                // JSON mode reserves stdout for Frontend Protocol frames.
                                                // Set the ordinary root quiet option as well as the live
                                                // logger because Config parses once during init and again
                                                // when the event loop starts.
                                                utils::Config::configRoot.getOption("--quiet")->add_result("true");
                                                logger::Logger::setQuiet(true);
                                            },
                                            "Emit compact Codex Frontend Protocol JSON on stdout",
                                            "",
                                            CLI::Validator{})
                                        ->trigger_on_parse();

    core::SNodeC::init(argc, argv);

    int result = 1;
    try {
        namespace client = apps::codex_backend_client;
        namespace frontend = ai::openai::codex::frontend;

        client::Presenter presenter(jsonOption->as<bool>() ? client::OutputMode::Json : client::OutputMode::Human);
        client::CommandParser parser;
        std::function<void()> requestShutdown;
        client::StdinReader* stdinReader = nullptr;
        net::un::stream::legacy::config::ConfigSocketClient* socketConfig = nullptr;
        bool stopping = false;

        client::ClientConnection connection(client::ClientConnectionCallbacks{
            .onConnected =
                [&presenter, &socketConfig]() {
                    presenter.connected(socketConfig != nullptr ? socketConfig->Remote::getSunPath() : client::defaultSocketPath());
                    if (presenter.outputMode() == client::OutputMode::Human) {
                        presenter.localMessage("enter 'help' for commands");
                    }
                },
            .onMessage =
                [&presenter](const frontend::ServerMessage& message) {
                    presenter.present(message);
                },
            .onProtocolError =
                [&presenter, &requestShutdown](const frontend::CodecError& error) {
                    presenter.error(std::string(frontend::toString(error.code)) + ": " + error.message);
                    if (error.closeConnection && requestShutdown) {
                        requestShutdown();
                    }
                },
            .onDisconnected =
                [&presenter, &stdinReader, &stopping]() {
                    presenter.disconnected();
                    stopping = true;
                    if (stdinReader != nullptr) {
                        stdinReader->stop();
                    }
                    core::SNodeC::stop();
                }});

        requestShutdown = [&connection, &stdinReader, &stopping]() {
            if (stopping) {
                return;
            }
            stopping = true;
            if (stdinReader != nullptr) {
                stdinReader->stop();
            }
            if (connection.connected()) {
                connection.disconnect();
            } else {
                core::SNodeC::stop();
            }
        };

        net::un::stream::legacy::SocketClient<client::CodexBackendClientSocketContextFactory, client::ClientConnection&> socketClient(
            "codex-backend-client", connection);
        socketClient.getConfig()->Remote::setSunPath(client::defaultSocketPath());
        socketConfig = socketClient.getConfig();

        client::StdinReader input(
            [&parser, &presenter, &connection, &requestShutdown](std::string line) {
                client::ParsedCommand parsed = parser.parse(line);
                std::visit(
                    [&presenter, &connection, &requestShutdown]<typename Command>(Command&& command) {
                        using T = std::remove_cvref_t<Command>;
                        if constexpr (std::is_same_v<T, client::NoopCommand>) {
                            return;
                        } else if constexpr (std::is_same_v<T, client::HelpCommand>) {
                            presenter.localMessage(client::CommandParser::helpText());
                        } else if constexpr (std::is_same_v<T, client::QuitCommand>) {
                            requestShutdown();
                        } else if constexpr (std::is_same_v<T, client::WatchCommand>) {
                            presenter.setWatchEnabled(command.enabled);
                            presenter.localMessage(command.enabled ? "watch on" : "watch off");
                        } else if constexpr (std::is_same_v<T, client::SendCommand>) {
                            if (!connection.send(command.message)) {
                                presenter.error("not connected; command was not sent");
                            }
                        } else {
                            presenter.error(command.message);
                        }
                    },
                    std::move(parsed));
            },
            [&requestShutdown]() {
                requestShutdown();
            },
            [&presenter, &requestShutdown](std::string message) {
                presenter.error(message);
                requestShutdown();
            });
        stdinReader = &input;

        socketClient.connect([&presenter, &requestShutdown, &socketConfig](const net::un::SocketAddress&, core::socket::State state) {
            if (state != core::socket::State::OK) {
                presenter.error("failed to connect to Unix socket " +
                                (socketConfig != nullptr ? socketConfig->Remote::getSunPath() : client::defaultSocketPath()));
                requestShutdown();
            }
        });

        result = core::SNodeC::start();
        input.stop();
        connection.disconnect();
    } catch (const std::exception& exception) {
        std::cerr << "codex-backend-client: " << exception.what() << '\n';
    } catch (...) {
        std::cerr << "codex-backend-client: unexpected fatal error\n";
    }

    core::SNodeC::free();
    return result;
}
