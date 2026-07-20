/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/Presenter.h"

#include "ai/openai/codex/frontend/Codec.h"

#include <cstddef>
#include <exception>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace apps::codex_backend_client {

    namespace {

        namespace frontend = ai::openai::codex::frontend;

        void writeLine(std::ostream& stream, std::string_view value) {
            stream << value;
            if (value.empty() || value.back() != '\n') {
                stream << '\n';
            }
            stream.flush();
        }

    } // namespace

    Presenter::Presenter(OutputMode mode)
        : Presenter(mode, std::cout, std::cerr) {
    }

    Presenter::Presenter(OutputMode mode, std::ostream& output, std::ostream& diagnostics)
        : mode(mode)
        , output(&output)
        , diagnostics(&diagnostics) {
    }

    void Presenter::present(const frontend::ServerMessage& message) {
        if (!watching && std::holds_alternative<frontend::EventBatch>(message)) {
            return;
        }

        if (mode == OutputMode::Json) {
            presentJson(message);
        } else {
            presentHuman(message);
        }
    }

    void Presenter::setWatchEnabled(bool enabled) noexcept {
        watching = enabled;
    }

    bool Presenter::watchEnabled() const noexcept {
        return watching;
    }

    OutputMode Presenter::outputMode() const noexcept {
        return mode;
    }

    void Presenter::connected(std::string_view socketPath) {
        std::string message = "connected to " + std::string(socketPath);
        if (mode == OutputMode::Human) {
            message += "; waiting for initial synchronization";
        }
        localMessage(message);
    }

    void Presenter::disconnected() {
        localMessage("disconnected");
    }

    void Presenter::localMessage(std::string_view message) {
        // Keep stdout machine-readable in JSON mode. Local lifecycle and
        // interactive diagnostics are not Frontend Protocol messages.
        writeLine(mode == OutputMode::Json ? *diagnostics : *output, message);
    }

    void Presenter::error(std::string_view message) {
        writeLine(*diagnostics, "codex-backend-client: " + std::string(message));
    }

    void Presenter::presentHuman(const frontend::ServerMessage& message) {
        try {
            std::visit(
                [this]<typename Message>(const Message& value) {
                    using T = std::remove_cvref_t<Message>;
                    if constexpr (std::is_same_v<T, frontend::Welcome>) {
                        *output << "welcome session=" << value.sessionId << " role=" << frontend::toString(value.role)
                                << " current-sequence=" << value.currentSequence.value()
                                << " sync-mode=" << frontend::toString(value.syncMode) << '\n';
                    } else if constexpr (std::is_same_v<T, frontend::SyncComplete>) {
                        *output << "sync.complete sequence=" << value.sequence.value() << '\n';
                    } else if constexpr (std::is_same_v<T, frontend::Snapshot>) {
                        *output << "snapshot sequence=" << value.sequence.value() << '\n' << value.state.dump(2) << '\n';
                    } else if constexpr (std::is_same_v<T, frontend::EventBatch>) {
                        *output << "events from=" << value.fromSequence.value() << " to=" << value.toSequence.value()
                                << " count=" << value.events.size() << '\n';
                        for (const frontend::FrontendEvent& event : value.events) {
                            *output << "  [" << event.sequence.value() << "] " << event.type << ' ' << event.data.dump() << '\n';
                        }
                    } else if constexpr (std::is_same_v<T, frontend::Response>) {
                        *output << "response request-id=" << value.requestId << " ok=" << (value.ok ? "true" : "false");
                        if (value.ok) {
                            if (value.result.has_value()) {
                                *output << " result=" << value.result->dump();
                            }
                        } else if (value.error.has_value()) {
                            *output << " error=" << frontend::toString(value.error->code) << " message=" << value.error->message;
                            if (value.error->details.has_value()) {
                                *output << " details=" << value.error->details->dump();
                            }
                        }
                        *output << '\n';
                    } else if constexpr (std::is_same_v<T, frontend::ProtocolErrorMessage>) {
                        *output << "protocol.error code=" << frontend::toString(value.code)
                                << " close=" << (value.closeConnection ? "true" : "false") << " message=" << value.message;
                        if (value.requestId.has_value()) {
                            *output << " request-id=" << *value.requestId;
                        }
                        if (!value.supportedVersions.empty()) {
                            *output << " supported-versions=";
                            for (std::size_t index = 0; index < value.supportedVersions.size(); ++index) {
                                if (index != 0) {
                                    *output << ',';
                                }
                                *output << value.supportedVersions[index];
                            }
                        }
                        if (value.details.has_value()) {
                            *output << " details=" << value.details->dump();
                        }
                        *output << '\n';
                    }
                },
                message);
            output->flush();
        } catch (const std::exception& exception) {
            error("failed to render frontend message: " + std::string(exception.what()));
        } catch (...) {
            error("failed to render frontend message");
        }
    }

    void Presenter::presentJson(const frontend::ServerMessage& message) {
        const auto serialized = frontend::Codec::serializeServer(message);
        if (!serialized) {
            error("failed to serialize frontend message: " + serialized.error().message);
            return;
        }
        writeLine(*output, serialized.value());
    }

} // namespace apps::codex_backend_client
