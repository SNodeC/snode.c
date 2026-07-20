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
#include "apps/codex-backend-client/CommandDrainController.h"
#include "apps/codex-backend-client/CommandParser.h"
#include "apps/codex-backend-client/Configuration.h"
#include "apps/codex-backend-client/Presenter.h"
#include "apps/codex-backend-client/StdinReader.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "log/Logger.h"
#include "net/un/SocketAddress.h"
#include "net/un/stream/legacy/SocketClient.h"
#include "utils/Config.h"

#include <exception>
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
        client::ClientConnection* connectionHandle = nullptr;
        client::StdinReader* stdinReader = nullptr;
        net::un::stream::legacy::config::ConfigSocketClient* socketConfig = nullptr;
        bool eventLoopRunning = false;
        bool exitScheduled = false;

        client::CommandDrainController lifecycle(
            client::CommandDrainCallbacks{.send =
                                              [&connectionHandle](const frontend::ClientMessage& message) {
                                                  return connectionHandle != nullptr && connectionHandle->send(message);
                                              },
                                          .requestExit =
                                              [&connectionHandle, &stdinReader, &eventLoopRunning, &exitScheduled]() {
                                                  if (stdinReader != nullptr) {
                                                      stdinReader->stop();
                                                  }
                                                  if (!eventLoopRunning || exitScheduled) {
                                                      return;
                                                  }
                                                  exitScheduled = true;
                                                  core::EventReceiver::atNextTick([&connectionHandle, &eventLoopRunning]() {
                                                      if (!eventLoopRunning) {
                                                          return;
                                                      }
                                                      if (connectionHandle != nullptr && connectionHandle->connected()) {
                                                          connectionHandle->disconnect();
                                                      } else {
                                                          core::SNodeC::stop();
                                                      }
                                                  });
                                              },
                                          .reportFailure =
                                              [&presenter](std::string message) {
                                                  presenter.error(message);
                                              }});

        client::ClientConnection connection(client::ClientConnectionCallbacks{
            .onConnected =
                [&presenter, &socketConfig, &lifecycle]() {
                    presenter.connected(socketConfig != nullptr ? socketConfig->Remote::getSunPath() : client::defaultSocketPath());
                    if (presenter.outputMode() == client::OutputMode::Human) {
                        presenter.localMessage("enter 'help' for commands");
                    }
                    lifecycle.connected();
                },
            .onMessage =
                [&presenter, &lifecycle](const frontend::ServerMessage& message) {
                    // Presentation is synchronous and flushes before lifecycle
                    // completion can schedule a controlled disconnect.
                    presenter.present(message);
                    lifecycle.receive(message);
                },
            .onProtocolError =
                [&lifecycle](const frontend::CodecError& error) {
                    lifecycle.protocolFailed(std::string(frontend::toString(error.code)) + ": " + error.message);
                },
            .onDisconnected =
                [&presenter, &lifecycle, &eventLoopRunning]() {
                    presenter.disconnected();
                    lifecycle.disconnected();
                    if (eventLoopRunning) {
                        core::SNodeC::stop();
                    }
                }});
        connectionHandle = &connection;

        net::un::stream::legacy::SocketClient<client::CodexBackendClientSocketContextFactory, client::ClientConnection&> socketClient(
            "codex-backend-client", connection);
        socketClient.getConfig()->Remote::setSunPath(client::defaultSocketPath());
        socketConfig = socketClient.getConfig();

        client::StdinReader input(
            [&parser, &presenter, &lifecycle](std::string line) {
                client::ParsedCommand parsed = parser.parse(line);
                std::visit(
                    [&presenter, &lifecycle]<typename Command>(Command&& command) {
                        using T = std::remove_cvref_t<Command>;
                        if constexpr (std::is_same_v<T, client::NoopCommand>) {
                            return;
                        } else if constexpr (std::is_same_v<T, client::HelpCommand>) {
                            presenter.localMessage(client::CommandParser::helpText());
                        } else if constexpr (std::is_same_v<T, client::QuitCommand>) {
                            lifecycle.quit();
                        } else if constexpr (std::is_same_v<T, client::WatchCommand>) {
                            presenter.setWatchEnabled(command.enabled);
                            presenter.localMessage(command.enabled ? "watch on" : "watch off");
                        } else if constexpr (std::is_same_v<T, client::SendCommand>) {
                            if (!lifecycle.enqueue(std::move(command.message)) && !lifecycle.failed()) {
                                presenter.error("command input is closed; command was not queued");
                            }
                        } else {
                            presenter.error(command.message);
                        }
                    },
                    std::move(parsed));
            },
            [&lifecycle]() {
                lifecycle.inputEof();
            },
            [&lifecycle](std::string message) {
                lifecycle.inputFailed(std::move(message));
            });
        stdinReader = &input;

        eventLoopRunning = true;
        socketClient.connect([&lifecycle, &socketConfig](const net::un::SocketAddress&, core::socket::State state) {
            if (state != core::socket::State::OK) {
                lifecycle.connectionFailed("failed to connect to Unix socket " +
                                           (socketConfig != nullptr ? socketConfig->Remote::getSunPath() : client::defaultSocketPath()));
            }
        });

        const int eventLoopResult = core::SNodeC::start();
        eventLoopRunning = false;
        input.stop();
        if (lifecycle.outcome() == client::CommandDrainController::Outcome::Running) {
            lifecycle.quit();
        }
        connection.disconnect();
        result = lifecycle.failed() ? 1 : eventLoopResult;
    } catch (const std::exception& exception) {
        std::cerr << "codex-backend-client: " << exception.what() << '\n';
    } catch (...) {
        std::cerr << "codex-backend-client: unexpected fatal error\n";
    }

    core::SNodeC::free();
    return result;
}
