/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/Presenter.h"

#include "ai/openai/codex/frontend/Codec.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace apps::codex_backend_client {

    namespace {

        namespace frontend = ai::openai::codex::frontend;

        constexpr std::size_t maximumInlineJsonBytes = 4096;
        constexpr std::size_t maximumSummaryIdentifiers = 20;
        constexpr std::size_t maximumSummaryIdentifierBytes = 80;

        void writeLine(std::ostream& stream, std::string_view value) {
            stream << value;
            if (value.empty() || value.back() != '\n') {
                stream << '\n';
            }
            stream.flush();
        }

        std::string jsonScalar(const nlohmann::json& object, std::string_view key, std::string fallback = "unknown") {
            if (!object.is_object()) {
                return fallback;
            }
            const auto found = object.find(key);
            if (found == object.end()) {
                return fallback;
            }
            if (found->is_string()) {
                return found->get<std::string>();
            }
            if (found->is_boolean()) {
                return found->get<bool>() ? "true" : "false";
            }
            if (found->is_number_unsigned()) {
                return std::to_string(found->get<std::uint64_t>());
            }
            if (found->is_number_integer()) {
                return std::to_string(found->get<std::int64_t>());
            }
            return fallback;
        }

        std::string boundedIdentifier(std::string value) {
            if (value.size() > maximumSummaryIdentifierBytes) {
                value.resize(maximumSummaryIdentifierBytes);
                value += "...";
            }
            return value;
        }

        std::string jsonArraySize(const nlohmann::json& object, std::string_view key) {
            if (!object.is_object()) {
                return "unknown";
            }
            const auto found = object.find(key);
            return found != object.end() && found->is_array() ? std::to_string(found->size()) : "unknown";
        }

        std::string threadListComplete(const nlohmann::json& state) {
            if (!state.is_object()) {
                return "unknown";
            }
            const auto threadList = state.find("threadList");
            return threadList != state.end() ? jsonScalar(*threadList, "complete") : "unknown";
        }

        std::string stateSummary(const nlohmann::json& state) {
            std::ostringstream summary;
            summary << "lifecycle=" << jsonScalar(state, "lifecycle") << " backend-revision=" << jsonScalar(state, "backendRevision")
                    << " threads=" << jsonArraySize(state, "threads") << " pending-requests=" << jsonArraySize(state, "pendingRequests")
                    << " controller=" << jsonScalar(state, "controllerSessionId", "none")
                    << " sessions=" << jsonArraySize(state, "sessions") << " thread-list-complete=" << threadListComplete(state);
            return summary.str();
        }

        std::string humanJson(const nlohmann::json& value) {
            if (value.is_object()) {
                if (value.contains("lifecycle") && value.contains("threads")) {
                    return stateSummary(value);
                }

                const auto threads = value.find("threads");
                if (threads != value.end() && threads->is_array()) {
                    std::ostringstream summary;
                    summary << "threads=" << threads->size();
                    if (!threads->empty()) {
                        summary << " ids=";
                        const std::size_t count = std::min(threads->size(), maximumSummaryIdentifiers);
                        for (std::size_t index = 0; index < count; ++index) {
                            if (index != 0) {
                                summary << ',';
                            }
                            summary << boundedIdentifier(jsonScalar((*threads)[index], "id"));
                        }
                        if (threads->size() > count) {
                            summary << ",...";
                        }
                    }
                    if (value.contains("nextCursor")) {
                        summary << " next-cursor=" << boundedIdentifier(jsonScalar(value, "nextCursor", "none"));
                    }
                    if (value.contains("backwardsCursor")) {
                        summary << " backwards-cursor=" << boundedIdentifier(jsonScalar(value, "backwardsCursor", "none"));
                    }
                    return summary.str();
                }

                const auto thread = value.find("thread");
                if (thread != value.end() && thread->is_object()) {
                    return "thread=" + boundedIdentifier(jsonScalar(*thread, "id")) +
                           " status=" + boundedIdentifier(jsonScalar(*thread, "status")) + " turns=" + jsonArraySize(*thread, "turns");
                }

                const auto turn = value.find("turn");
                if (turn != value.end() && turn->is_object()) {
                    return "turn=" + boundedIdentifier(jsonScalar(*turn, "id")) +
                           " thread=" + boundedIdentifier(jsonScalar(*turn, "threadId")) +
                           " status=" + boundedIdentifier(jsonScalar(*turn, "status")) + " items=" + jsonArraySize(*turn, "items");
                }
            }

            std::string encoded = value.dump();
            if (encoded.size() <= maximumInlineJsonBytes) {
                return encoded;
            }
            return "<json omitted; bytes=" + std::to_string(encoded.size()) + "; use --json for complete protocol data>";
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
                        *output << "snapshot sequence=" << value.sequence.value() << ' ' << stateSummary(value.state) << '\n';
                    } else if constexpr (std::is_same_v<T, frontend::EventBatch>) {
                        *output << "events from=" << value.fromSequence.value() << " to=" << value.toSequence.value()
                                << " count=" << value.events.size() << '\n';
                        for (const frontend::FrontendEvent& event : value.events) {
                            *output << "  [" << event.sequence.value() << "] " << event.type << ' ' << humanJson(event.data) << '\n';
                        }
                    } else if constexpr (std::is_same_v<T, frontend::Response>) {
                        *output << "response request-id=" << value.requestId << " ok=" << (value.ok ? "true" : "false");
                        if (value.ok) {
                            if (value.result.has_value()) {
                                *output << " result=" << humanJson(*value.result);
                            }
                        } else if (value.error.has_value()) {
                            *output << " error=" << frontend::toString(value.error->code) << " message=" << value.error->message;
                            if (value.error->details.has_value()) {
                                *output << " details=" << humanJson(*value.error->details);
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
                            *output << " details=" << humanJson(*value.details);
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
