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

    constexpr const char* SyntheticNotificationSecret = "synthetic-notification-secret";

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

    template <typename Notification>
    bool verifyPreserved(tests::support::TestResult& result,
                         const codex::Notification& wire,
                         const typed::Event& event) {
        const auto* decoded = std::get_if<Notification>(&event);
        result.expectTrue(decoded && decoded->raw == wire.raw,
                          wire.method + " typed notification retains the complete JSON-RPC envelope internally");

        backend::Reducer reducer;
        backend::BackendState state;
        const backend::Snapshot before = backend::makeSnapshot(state);
        const std::vector<backend::BackendEvent> translated = reducer.translate(event);
        const auto* extension =
            translated.size() == 1 ? std::get_if<backend::CodexExtensionReceived>(&translated.front()) : nullptr;

        const bool exactTranslation =
            extension && extension->method == wire.method && extension->payload == wire.params &&
            !extension->payload.contains("futureEnvelopeOnly") &&
            extension->payload.dump().find("mustNotBecomeParams") == std::string::npos;
        result.expectTrue(exactTranslation,
                          wire.method + " translation preserves only params, never the complete JSON-RPC envelope");

        if (extension) {
            const backend::Reduction reduction = reducer.apply(state, *extension);
            result.expectTrue(reduction.changed && !reduction.flushImmediately,
                              wire.method + " changes only the bounded extension history");
        }

        const backend::ExtensionRecord* retained =
            state.recentExtensions.size() == 1 ? &state.recentExtensions.front() : nullptr;
        result.expectTrue(retained && retained->method == wire.method && retained->payload == wire.params &&
                              !retained->payload.contains("futureEnvelopeOnly"),
                          wire.method + " bounded BackendCore preservation retains exact params and method");

        const backend::Snapshot after = backend::makeSnapshot(state);
        result.expectTrue(withoutExtensions(after) == before,
                          wire.method + " invents no account, authentication, or rate-limit canonical state");
        return decoded && exactTranslation && retained;
    }

    void testAllAccountNotifications(tests::support::TestResult& result) {
        const codex::Notification login =
            notification("account/login/completed",
                         {{"error", nullptr}, {"loginId", "login-preserved"}, {"success", true}, {"futureParam", 1}},
                         1);
        const codex::Notification rateLimits =
            notification("account/rateLimits/updated",
                         {{"rateLimits",
                           {{"credits", nullptr},
                            {"individualLimit", nullptr},
                            {"limitId", "primary"},
                            {"limitName", nullptr},
                            {"planType", "future-plan"},
                            {"primary", {{"resetsAt", nullptr}, {"usedPercent", 25}, {"windowDurationMins", 60}}},
                            {"rateLimitReachedType", nullptr},
                            {"secondary", nullptr}}},
                          {"futureParam", 2}},
                         2);
        const codex::Notification updated =
            notification("account/updated",
                         {{"authMode", "future-auth-mode"},
                          {"planType", nullptr},
                          {"accessToken", SyntheticNotificationSecret},
                          {"futureParam", 3}},
                         3);

        const typed::Event loginEvent = detail::decodeEvent(login);
        const typed::Event rateLimitEvent = detail::decodeEvent(rateLimits);
        const typed::Event updatedEvent = detail::decodeEvent(updated);

        std::size_t exact = 0;
        exact += verifyPreserved<typed::AccountLoginCompletedNotification>(result, login, loginEvent) ? 1U : 0U;
        exact += verifyPreserved<typed::AccountRateLimitsUpdatedNotification>(result, rateLimits, rateLimitEvent) ? 1U : 0U;
        exact += verifyPreserved<typed::AccountUpdatedNotification>(result, updated, updatedEvent) ? 1U : 0U;
        result.expectTrue(exact == 3,
                          "all three B2 account notifications decode and use the one bounded params-only preservation path");
    }

    void testFrontendSnapshotRedaction(tests::support::TestResult& result) {
        const codex::Notification updated =
            notification("account/updated",
                         {{"authMode", "chatgpt"},
                          {"planType", "plus"},
                          {"nested", {{"accessToken", SyntheticNotificationSecret}}},
                          {"safe", "visible"}},
                         4);
        const typed::Event event = detail::decodeEvent(updated);

        backend::Reducer reducer;
        backend::BackendState state;
        const std::vector<backend::BackendEvent> translated = reducer.translate(event);
        if (translated.size() == 1) {
            reducer.apply(state, translated.front());
        }

        const backend::ExtensionRecord* canonical =
            state.recentExtensions.size() == 1 ? &state.recentExtensions.front() : nullptr;
        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        const backend::ExtensionSnapshot* projected =
            snapshot.recentExtensions.size() == 1 ? &snapshot.recentExtensions.front() : nullptr;
        const std::string frontendPayload = projected ? projected->payload.dump() : std::string{};

        result.expectTrue(canonical && canonical->payload == updated.params,
                          "bounded internal extension state retains the exact typed notification params before projection");
        result.expectTrue(projected && projected->method == "account/updated" && projected->sensitiveFieldsRedacted &&
                              projected->payload.value("safe", "") == "visible" &&
                              frontendPayload.find(SyntheticNotificationSecret) == std::string::npos &&
                              frontendPayload.find("futureEnvelopeOnly") == std::string::npos,
                          "frontend snapshot recursively redacts credential-shaped values and never nests envelope fields under params");
    }

    void testActualFrontendAdapterAndCodecParity(tests::support::TestResult& result) {
        const codex::Notification updated =
            notification("account/updated",
                         {{"authMode", "chatgpt"},
                          {"planType", "plus"},
                          {"nested", {{"accessToken", SyntheticNotificationSecret}}},
                          {"safe", "visible"}},
                         5);

        const typed::Event decodedEvent = detail::decodeEvent(updated);
        const auto* decoded = std::get_if<typed::AccountUpdatedNotification>(&decodedEvent);
        result.expectTrue(decoded && decoded->raw == updated.raw,
                          "the adapter parity input retains the complete envelope in the typed notification raw member");

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
            "the actual Frontend Protocol adapter accepts the v1 hello");

        backendCore.start();
        core::EventReceiver::atNextTick([&outbound, &transport, &updated]() {
            outbound.clear();
            transport->inject(updated.raw);
        });

        std::function<void(std::size_t)> waitForExtension;
        waitForExtension = [&](std::size_t remaining) {
            const bool observed = std::any_of(outbound.begin(), outbound.end(), [](const frontend::OutboundMessage& message) {
                const auto* batch = std::get_if<frontend::EventBatch>(&message.message);
                return batch && std::any_of(batch->events.begin(), batch->events.end(), [](const frontend::FrontendEvent& event) {
                           return event.type == "codex.extension" &&
                                  event.data.value("method", "") == "account/updated";
                       });
            });
            if (observed || remaining == 0) {
                backendCore.stop();
                core::SNodeC::stop();
                return;
            }
            adapter.flush();
            core::EventReceiver::atNextTick([&waitForExtension, remaining]() {
                waitForExtension(remaining - 1);
            });
        };
        core::EventReceiver::atNextTick([&waitForExtension]() {
            waitForExtension(64);
        });
        const int eventLoopResult = core::SNodeC::start(utils::Timeval({2, 0}));
        result.expectEqual(0, eventLoopResult, "the actual frontend adapter parity event loop exits cleanly");

        const backend::BackendState canonical = backendCore.state();
        const backend::ExtensionRecord* retained =
            canonical.recentExtensions.size() == 1 ? &canonical.recentExtensions.front() : nullptr;
        result.expectTrue(retained && retained->method == updated.method && retained->payload == updated.params,
                          "BackendCore receives the typed account notification as exact params-only extension state");

        const backend::ExtensionSnapshot expected =
            retained ? backend::makeExtensionSnapshot(*retained) : backend::ExtensionSnapshot{};
        const frontend::OutboundMessage* encodedBatch = nullptr;
        const frontend::FrontendEvent* encodedEvent = nullptr;
        for (const frontend::OutboundMessage& message : outbound) {
            if (const auto* batch = std::get_if<frontend::EventBatch>(&message.message)) {
                for (const frontend::FrontendEvent& event : batch->events) {
                    if (event.type == "codex.extension" && event.data.value("method", "") == updated.method) {
                        encodedBatch = &message;
                        encodedEvent = &event;
                    }
                }
            }
        }

        const bool exactFrontendParams =
            encodedEvent && retained && encodedEvent->data.value("params", codex::Json{}) == expected.payload &&
            encodedEvent->data.value("sensitiveFieldsRedacted", false) &&
            encodedEvent->data.at("params").value("safe", "") == "visible" &&
            encodedEvent->data.at("params").dump().find(SyntheticNotificationSecret) == std::string::npos &&
            encodedEvent->data.at("params").dump().find("futureEnvelopeOnly") == std::string::npos &&
            !encodedEvent->data.at("params").contains("jsonrpc") && !encodedEvent->data.at("params").contains("method");
        result.expectTrue(
            exactFrontendParams,
            "actual frontend codex.extension.params equals recursively sanitized notification params and contains no envelope fields");

        codex::Json compact;
        if (encodedBatch) {
            compact = codex::Json::parse(encodedBatch->compactJson, nullptr, false);
        }
        const codex::Json* compactEvent = nullptr;
        if (!compact.is_discarded() && compact.contains("events") && compact["events"].is_array()) {
            for (const codex::Json& candidate : compact["events"]) {
                if (candidate.value("type", "") == "codex.extension" && candidate.contains("data") &&
                    candidate["data"].value("method", "") == updated.method) {
                    compactEvent = &candidate;
                }
            }
        }
        const bool unchangedWireShape =
            encodedBatch && encodedEvent && !compact.is_discarded() && compact.value("protocol", "") == frontend::ProtocolIdentity &&
            compact.value("version", 0) == frontend::ProtocolVersion && compact.value("kind", "") == "events" &&
            compact.size() == 6 && compact.contains("fromSequence") && compact.contains("toSequence") && compactEvent &&
            compactEvent->size() == 3 && compactEvent->contains("sequence") && compactEvent->at("data") == encodedEvent->data &&
            encodedEvent->data.size() == 3 && encodedEvent->data.contains("method") && encodedEvent->data.contains("params") &&
            encodedEvent->data.contains("sensitiveFieldsRedacted") &&
            encodedBatch->compactJson.find(SyntheticNotificationSecret) == std::string::npos;
        result.expectTrue(unchangedWireShape,
                          "actual Frontend Protocol v1 codec emits the existing codex.extension event shape without secret data");
    }
} // namespace

int main(int argc, char* argv[]) {
    static_assert(std::is_constructible_v<typed::Event, typed::AccountLoginCompletedNotification>);
    static_assert(std::is_constructible_v<typed::Event, typed::AccountRateLimitsUpdatedNotification>);
    static_assert(std::is_constructible_v<typed::Event, typed::AccountUpdatedNotification>);

    core::SNodeC::init(argc, argv);
    tests::support::TestResult result;
    testAllAccountNotifications(result);
    testFrontendSnapshotRedaction(result);
    testActualFrontendAdapterAndCodecParity(result);
    const int returnCode = result.processResult();
    core::SNodeC::free();
    return returnCode;
}
