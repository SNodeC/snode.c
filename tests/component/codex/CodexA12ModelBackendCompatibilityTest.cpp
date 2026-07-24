/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "CodexBackendTestSupport.h"
#include "ai/openai/codex/backend/BackendCore.h"
#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/frontend/BackendAdapter.h"
#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/typed/Events.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace frontend = ai::openai::codex::frontend;
    namespace typed = ai::openai::codex::typed;

    using FakeBackendCore = backend::BackendCore<tests::codex::FakeAppServerClient>;

    backend::Snapshot withoutExtensions(backend::Snapshot snapshot) {
        snapshot.recentExtensions.clear();
        snapshot.omittedRecentExtensions = 0;
        return snapshot;
    }

    codex::Notification notification(std::string method, codex::Json params, std::size_t sequence) {
        codex::Json raw{
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params},
            {"futureEnvelopeOnly", {{"sequence", sequence}, {"mustNotBecomeParams", true}}},
        };
        return {std::move(method), std::move(params), std::move(raw)};
    }

    const backend::ModelRerouted* translatedReroute(const std::vector<backend::BackendEvent>& events) {
        return events.size() == 1 ? std::get_if<backend::ModelRerouted>(&events.front()) : nullptr;
    }

    void testModelReroutedCompatibility(tests::support::TestResult& result) {
        const codex::Notification wire =
            notification("model/rerouted",
                         {{"fromModel", "model-before"},
                          {"reason", "futureRerouteReason"},
                          {"threadId", "thread-reroute"},
                          {"toModel", "model-after"},
                          {"turnId", "turn-reroute"}},
                         1);
        const typed::Event decodedEvent = detail::decodeEvent(wire);
        const auto* decoded = std::get_if<typed::ModelRerouted>(&decodedEvent);
        const typed::ModelReroutedNotification* canonical =
            decoded && decoded->canonical ? &*decoded->canonical : nullptr;

        const bool exactCanonical =
            decoded && canonical && decoded->threadId.value == "thread-reroute" &&
            decoded->turnId.value == "turn-reroute" && decoded->from.value == "model-before" &&
            decoded->to.value == "model-after" && decoded->reason == "futureRerouteReason" &&
            decoded->raw == wire.raw && canonical->fromModel == decoded->from &&
            canonical->toModel == decoded->to && canonical->reason.value == decoded->reason &&
            !canonical->reason.isKnown() && canonical->raw == wire.raw &&
            canonical->diagnostics.size() == 1 &&
            canonical->diagnostics.front().kind == typed::DecodeIssueKind::UnknownEnumValue &&
            canonical->diagnostics.front().severity == typed::DecodeIssueSeverity::ForwardCompatibility &&
            canonical->diagnostics.front().surface == "ModelRerouteReason" &&
            canonical->diagnostics.front().fieldPath == "$.params.reason";
        result.expectTrue(
            exactCanonical,
            "schema-complete model/rerouted retains the legacy view and exact future-reason canonical diagnostics");

        typed::ModelRerouted legacy{
            .threadId = typed::ThreadId{"thread-reroute"},
            .turnId = typed::TurnId{"turn-reroute"},
            .from = typed::ModelId{"model-before"},
            .to = typed::ModelId{"model-after"},
            .reason = "futureRerouteReason",
            .raw = wire.raw,
        };
        backend::Reducer reducer;
        const std::vector<backend::BackendEvent> decodedTranslation = reducer.translate(decodedEvent);
        const std::vector<backend::BackendEvent> legacyTranslation =
            reducer.translate(typed::Event{std::move(legacy)});
        const backend::ModelRerouted* decodedBackend = translatedReroute(decodedTranslation);
        const backend::ModelRerouted* legacyBackend = translatedReroute(legacyTranslation);
        result.expectTrue(
            decodedBackend && legacyBackend &&
                decodedBackend->threadId == legacyBackend->threadId &&
                decodedBackend->turnId == legacyBackend->turnId &&
                decodedBackend->from == legacyBackend->from &&
                decodedBackend->to == legacyBackend->to &&
                decodedBackend->reason == legacyBackend->reason &&
                decodedBackend->threadId.value == "thread-reroute" &&
                decodedBackend->turnId.value == "turn-reroute" &&
                decodedBackend->from.value == "model-before" &&
                decodedBackend->to.value == "model-after" &&
                decodedBackend->reason == "futureRerouteReason",
            "canonical and pre-A1.2 model/rerouted views translate to identical existing BackendCore semantics");

        backend::BackendState decodedState;
        backend::BackendState legacyState;
        if (decodedBackend) {
            reducer.apply(decodedState, *decodedBackend);
        }
        if (legacyBackend) {
            reducer.apply(legacyState, *legacyBackend);
        }
        const auto decodedThread = decodedState.threads.find("thread-reroute");
        const auto legacyThread = legacyState.threads.find("thread-reroute");
        const backend::TurnState* decodedTurn =
            decodedThread != decodedState.threads.end()
                    ? [&]() -> const backend::TurnState* {
                          const auto turn = decodedThread->second.turns.find("turn-reroute");
                          return turn == decodedThread->second.turns.end() ? nullptr : &turn->second;
                      }()
                    : nullptr;
        const backend::TurnState* legacyTurn =
            legacyThread != legacyState.threads.end()
                    ? [&]() -> const backend::TurnState* {
                          const auto turn = legacyThread->second.turns.find("turn-reroute");
                          return turn == legacyThread->second.turns.end() ? nullptr : &turn->second;
                      }()
                    : nullptr;
        result.expectTrue(
            decodedTurn && legacyTurn && decodedTurn->modelReroutes.size() == 1 &&
                legacyTurn->modelReroutes.size() == 1 &&
                decodedTurn->modelReroutes.front().from.value == "model-before" &&
                decodedTurn->modelReroutes.front().to.value == "model-after" &&
                decodedTurn->modelReroutes.front().reason == "futureRerouteReason" &&
                decodedTurn->modelReroutes.front().from == legacyTurn->modelReroutes.front().from &&
                decodedTurn->modelReroutes.front().to == legacyTurn->modelReroutes.front().to &&
                decodedTurn->modelReroutes.front().reason == legacyTurn->modelReroutes.front().reason &&
                backend::makeSnapshot(decodedState) == backend::makeSnapshot(legacyState),
            "model/rerouted keeps the exact thread/turn association, from/to/reason record, and frontend snapshot behavior");

        backend::BackendState boundedState;
        for (std::size_t index = 0; index < 65; ++index) {
            typed::ModelRerouted value{
                .threadId = typed::ThreadId{"thread-bounded"},
                .turnId = typed::TurnId{"turn-bounded"},
                .from = typed::ModelId{"model-" + std::to_string(index)},
                .to = typed::ModelId{"model-" + std::to_string(index + 1)},
                .reason = "future-reason-" + std::to_string(index),
                .raw = codex::Json::object(),
            };
            const std::vector<backend::BackendEvent> translated =
                reducer.translate(typed::Event{std::move(value)});
            if (const backend::ModelRerouted* reroute = translatedReroute(translated)) {
                reducer.apply(boundedState, *reroute);
            }
        }
        const auto boundedThread = boundedState.threads.find("thread-bounded");
        const backend::TurnState* boundedTurn =
            boundedThread != boundedState.threads.end()
                    ? [&]() -> const backend::TurnState* {
                          const auto turn = boundedThread->second.turns.find("turn-bounded");
                          return turn == boundedThread->second.turns.end() ? nullptr : &turn->second;
                      }()
                    : nullptr;
        result.expectTrue(
            boundedTurn && boundedTurn->modelReroutes.size() == 64 &&
                boundedTurn->modelReroutes.front().from.value == "model-1" &&
                boundedTurn->modelReroutes.front().reason == "future-reason-1" &&
                boundedTurn->modelReroutes.back().to.value == "model-65" &&
                boundedTurn->modelReroutes.back().reason == "future-reason-64",
            "model/rerouted preserves the existing exact 64-record bounded-history behavior for future reasons");
    }

    template <typename Notification>
    bool verifyPreserved(tests::support::TestResult& result,
                         backend::Reducer& reducer,
                         backend::BackendState& state,
                         const codex::Notification& wire,
                         const typed::Event& event) {
        const auto* decoded = std::get_if<Notification>(&event);
        result.expectTrue(decoded && decoded->raw == wire.raw,
                          wire.method + " typed notification retains the complete JSON-RPC envelope internally");
        const std::vector<backend::BackendEvent> translated = reducer.translate(event);
        const auto* extension =
            translated.size() == 1 ? std::get_if<backend::CodexExtensionReceived>(&translated.front()) : nullptr;
        result.expectTrue(
            extension && extension->method == wire.method && extension->payload == wire.params &&
                !extension->payload.contains("futureEnvelopeOnly") &&
                extension->payload.dump().find("mustNotBecomeParams") == std::string::npos,
            wire.method + " translation preserves exact params rather than the complete envelope");
        if (extension) {
            reducer.apply(state, *extension);
        }
        const backend::ExtensionRecord* retained =
            state.recentExtensions.empty() ? nullptr : &state.recentExtensions.back();
        result.expectTrue(retained && retained->method == wire.method && retained->payload == wire.params &&
                              !retained->payload.contains("futureEnvelopeOnly"),
                          wire.method + " uses the existing bounded params-only extension state");
        return decoded && extension && retained;
    }

    void testUnmodeledPreservation(tests::support::TestResult& result) {
        const codex::Notification safety =
            notification("model/safetyBuffering/updated",
                         {{"fasterModel", nullptr},
                          {"model", "model-buffering"},
                          {"reasons", codex::Json::array({"safety-review"})},
                          {"showBufferingUi", true},
                          {"threadId", "thread-preserve"},
                          {"turnId", "turn-preserve"},
                          {"useCases", codex::Json::array({"cyber"})},
                          {"futureParam", 2}},
                         2);
        const codex::Notification verification =
            notification("model/verification",
                         {{"threadId", "thread-preserve"},
                          {"turnId", "turn-preserve"},
                          {"verifications", codex::Json::array({"trustedAccessForCyber", "futureVerification"})},
                          {"futureParam", 3}},
                         3);
        const typed::Event safetyEvent = detail::decodeEvent(safety);
        const typed::Event verificationEvent = detail::decodeEvent(verification);

        backend::Reducer reducer;
        backend::BackendState state;
        const backend::Snapshot before = backend::makeSnapshot(state);
        std::size_t exact = 0;
        exact += verifyPreserved<typed::ModelSafetyBufferingUpdatedNotification>(
                     result, reducer, state, safety, safetyEvent)
                     ? 1U
                     : 0U;
        exact += verifyPreserved<typed::ModelVerificationNotification>(
                     result, reducer, state, verification, verificationEvent)
                     ? 1U
                     : 0U;
        result.expectTrue(exact == 2 && state.recentExtensions.size() == 2 &&
                              withoutExtensions(backend::makeSnapshot(state)) == before &&
                              state.threads.empty(),
                          "both new B3 notifications remain bounded extensions and invent no canonical model state");

        backend::BackendState bounded;
        for (std::size_t index = 0; index < 65; ++index) {
            codex::Notification repeated =
                notification("model/verification",
                             {{"threadId", "thread-preserve"},
                              {"turnId", "turn-preserve"},
                              {"verifications", codex::Json::array({"trustedAccessForCyber"})},
                              {"sequence", index}},
                             index);
            const std::vector<backend::BackendEvent> translated =
                reducer.translate(detail::decodeEvent(repeated));
            if (translated.size() == 1) {
                reducer.apply(bounded, translated.front());
            }
        }
        result.expectTrue(bounded.recentExtensions.size() == 64 &&
                              bounded.recentExtensions.front().payload.value("sequence", 0U) == 1U &&
                              bounded.recentExtensions.back().payload.value("sequence", 0U) == 64U,
                          "new model notifications inherit the existing exact 64-record extension bound");
    }

    void testActualFrontendAdapterParity(tests::support::TestResult& result) {
        const codex::Notification rerouted =
            notification("model/rerouted",
                         {{"fromModel", "model-before"},
                          {"reason", "futureRerouteReason"},
                          {"threadId", "thread-adapter"},
                          {"toModel", "model-after"},
                          {"turnId", "turn-adapter"}},
                         4);
        const codex::Notification safety =
            notification("model/safetyBuffering/updated",
                         {{"fasterModel", "model-faster"},
                          {"model", "model-buffering"},
                          {"reasons", codex::Json::array({"review"})},
                          {"showBufferingUi", false},
                          {"threadId", "thread-adapter"},
                          {"turnId", "turn-adapter"},
                          {"useCases", codex::Json::array({"cyber"})},
                          {"futureParam", 5}},
                         5);
        const codex::Notification verification =
            notification("model/verification",
                         {{"threadId", "thread-adapter"},
                          {"turnId", "turn-adapter"},
                          {"verifications", codex::Json::array({"futureVerification"})},
                          {"futureParam", 6}},
                         6);

        auto transport = std::make_shared<tests::codex::FakeTransportState>();
        tests::codex::installInitializingFake(
            transport,
            [](const codex::Json& message, const detail::TransportCallbacks& callbacks) {
                const auto method = message.find("method");
                const auto id = message.find("id");
                if (method != message.end() && method->is_string() && *method == "thread/list" && id != message.end()) {
                    tests::codex::inject(
                        callbacks,
                        codex::Json{
                            {"id", *id},
                            {"result", {{"data", codex::Json::array()}, {"nextCursor", nullptr}, {"backwardsCursor", nullptr}}},
                        });
                }
            });

        FakeBackendCore backendCore(transport);
        frontend::BackendAdapter adapter(backendCore);
        std::vector<frontend::OutboundMessage> outbound;
        frontend::FrontendConnection connection = adapter.openConnection(
            {[&outbound](const frontend::OutboundMessage& message) {
                 outbound.push_back(message);
                 return true;
             },
             [](const std::string&) {}});
        result.expectTrue(
            connection.receive(frontend::ClientMessage{frontend::Hello{std::nullopt, frontend::Json::object()}}).accepted(),
            "the actual Frontend Protocol adapter accepts the v1 hello for B3 parity");

        backendCore.start();
        core::EventReceiver::atNextTick([&]() {
            outbound.clear();
            transport->inject(rerouted.raw);
            transport->inject(safety.raw);
            transport->inject(verification.raw);
        });

        std::function<void(std::size_t)> waitForEvents;
        waitForEvents = [&](std::size_t remaining) {
            std::size_t extensionCount = 0;
            bool turnObserved = false;
            for (const frontend::OutboundMessage& message : outbound) {
                if (const auto* batch = std::get_if<frontend::EventBatch>(&message.message)) {
                    for (const frontend::FrontendEvent& event : batch->events) {
                        extensionCount += event.type == "codex.extension" ? 1U : 0U;
                        turnObserved = turnObserved || event.type == "turn.updated";
                    }
                }
            }
            if ((extensionCount >= 2 && turnObserved) || remaining == 0) {
                backendCore.stop();
                core::SNodeC::stop();
                return;
            }
            adapter.flush();
            core::EventReceiver::atNextTick([&waitForEvents, remaining]() {
                waitForEvents(remaining - 1);
            });
        };
        core::EventReceiver::atNextTick([&waitForEvents]() {
            waitForEvents(64);
        });
        const int eventLoopResult = core::SNodeC::start(utils::Timeval({3, 0}));
        result.expectEqual(0, eventLoopResult, "the actual B3 frontend adapter parity event loop exits cleanly");

        const backend::BackendState canonical = backendCore.state();
        const auto thread = canonical.threads.find("thread-adapter");
        const backend::TurnState* turn =
            thread != canonical.threads.end()
                    ? [&]() -> const backend::TurnState* {
                          const auto found = thread->second.turns.find("turn-adapter");
                          return found == thread->second.turns.end() ? nullptr : &found->second;
                      }()
                    : nullptr;
        result.expectTrue(turn && turn->modelReroutes.size() == 1 &&
                              turn->modelReroutes.front().from.value == "model-before" &&
                              turn->modelReroutes.front().to.value == "model-after" &&
                              turn->modelReroutes.front().reason == "futureRerouteReason" &&
                              canonical.recentExtensions.size() == 2,
                          "actual BackendCore preserves the existing reroute record and exactly two new extension records");

        std::vector<const frontend::FrontendEvent*> extensions;
        bool turnObserved = false;
        bool v1WireShape = true;
        bool rerouteNotExposedAsExtension = true;
        bool noNewRerouteField = true;
        for (const frontend::OutboundMessage& message : outbound) {
            const codex::Json compact = codex::Json::parse(message.compactJson, nullptr, false);
            v1WireShape = v1WireShape && !compact.is_discarded() &&
                          compact.value("protocol", "") == frontend::ProtocolIdentity &&
                          compact.value("version", 0) == frontend::ProtocolVersion;
            if (const auto* batch = std::get_if<frontend::EventBatch>(&message.message)) {
                for (const frontend::FrontendEvent& event : batch->events) {
                    turnObserved = turnObserved || event.type == "turn.updated";
                    if (event.type == "codex.extension") {
                        rerouteNotExposedAsExtension =
                            rerouteNotExposedAsExtension &&
                            event.data.value("method", "") != "model/rerouted";
                        extensions.push_back(&event);
                    }
                    noNewRerouteField =
                        noNewRerouteField && event.data.dump().find("modelReroutes") == std::string::npos &&
                        event.data.dump().find("fromModel") == std::string::npos &&
                        event.data.dump().find("toModel") == std::string::npos;
                }
            }
        }

        bool exactExtensions = extensions.size() == 2 && canonical.recentExtensions.size() == 2;
        result.expectTrue(extensions.size() == 2,
                          "actual frontend v1 emits exactly two B3 codex.extension events");
        for (const frontend::FrontendEvent* event : extensions) {
            const std::string method = event->data.value("method", "");
            const auto retained = std::find_if(
                canonical.recentExtensions.begin(),
                canonical.recentExtensions.end(),
                [&](const backend::ExtensionRecord& extension) {
                    return extension.method == method;
                });
            if (retained == canonical.recentExtensions.end()) {
                exactExtensions = false;
                result.expectTrue(false, method + " frontend extension has a matching bounded BackendCore record");
                continue;
            }
            const backend::ExtensionSnapshot expected = backend::makeExtensionSnapshot(*retained);
            const codex::Json params = event->data.value("params", codex::Json{});
            const bool sanitizedParity = params == expected.payload;
            const bool internalParity = params == retained->payload;
            const bool paramsOnly = !params.contains("jsonrpc") && !params.contains("method") &&
                                    !params.contains("futureEnvelopeOnly") &&
                                    params.dump().find("mustNotBecomeParams") == std::string::npos;
            const bool stableShape =
                event->data.size() == 2 && event->data.contains("method") &&
                event->data.contains("params") && !event->data.contains("sensitiveFieldsRedacted");
            result.expectTrue(sanitizedParity,
                              method + " frontend params equal the sanitized bounded extension payload");
            result.expectTrue(internalParity,
                              method + " safe frontend params remain equal to exact internal params");
            result.expectTrue(paramsOnly,
                              method + " frontend params exclude every JSON-RPC envelope-only field");
            result.expectTrue(stableShape,
                              method + " safe frontend extension retains the exact existing method/params data shape");
            exactExtensions = exactExtensions && sanitizedParity && internalParity && paramsOnly && stableShape;
        }
        result.expectTrue(exactExtensions,
                          "actual frontend v1 codex.extension.params exactly match the two sanitized BackendCore params values");
        result.expectTrue(turnObserved,
                          "actual frontend v1 retains the existing model/rerouted turn.updated behavior");
        result.expectTrue(rerouteNotExposedAsExtension,
                          "actual frontend v1 does not reclassify modeled model/rerouted as an extension");
        result.expectTrue(noNewRerouteField,
                          "actual frontend v1 adds no model-reroute snapshot or event field");
        result.expectTrue(v1WireShape,
                          "actual frontend adapter output retains the existing protocol identity and version");
    }
} // namespace

int main(int argc, char* argv[]) {
    static_assert(std::is_constructible_v<typed::Event, typed::ModelRerouted>);
    static_assert(std::is_constructible_v<typed::Event, typed::ModelSafetyBufferingUpdatedNotification>);
    static_assert(std::is_constructible_v<typed::Event, typed::ModelVerificationNotification>);

    core::SNodeC::init(argc, argv);
    tests::support::TestResult result;
    testModelReroutedCompatibility(result);
    testUnmodeledPreservation(result);
    testActualFrontendAdapterParity(result);
    const int returnCode = result.processResult();
    core::SNodeC::free();
    return returnCode;
}
