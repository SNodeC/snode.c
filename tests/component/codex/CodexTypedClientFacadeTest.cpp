/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/Transport.h"
#include "ai/openai/codex/typed/Client.h"
#include "support/TestResult.h"

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;
    namespace typed = codex::typed;

    struct GenericResultHandler {
        template <typename Result>
        void operator()(const Result&) const {
        }
    };

    template <typename Client>
    concept HasDirectAccountsAccessor = requires(Client& client) {
        client.accounts();
    };

    template <typename Client>
    concept HasDirectModelsAccessor = requires(Client& client) {
        client.models();
    };

    using LegacyThreadStartMember =
        typed::Threads::Submission (typed::Threads::*)(typed::ThreadStartOptions, typed::Threads::ThreadResultHandler);
    using LegacyThreadResumeMember = typed::Threads::Submission (typed::Threads::*)(
        typed::ThreadId, typed::ThreadResumeOptions, typed::Threads::ThreadResultHandler);
    using LegacyThreadListMember =
        typed::Threads::Submission (typed::Threads::*)(typed::ThreadListOptions, typed::Threads::ThreadListResultHandler);
    using LegacyThreadReadMember =
        typed::Threads::Submission (typed::Threads::*)(typed::ThreadId, typed::Threads::ThreadResultHandler);
    using LegacyThreadReadOptionsMember = typed::Threads::Submission (typed::Threads::*)(
        typed::ThreadId, typed::ThreadReadOptions, typed::Threads::ThreadResultHandler);
    using LegacyTurnStartMember = typed::Turns::Submission (typed::Turns::*)(
        typed::ThreadId, std::vector<typed::TurnInput>, typed::TurnStartOptions, typed::Turns::TurnResultHandler);
    using LegacyTurnInterruptMember =
        typed::Turns::Submission (typed::Turns::*)(typed::ThreadId, typed::TurnId, typed::Turns::InterruptResultHandler);

    static_assert(std::same_as<decltype(static_cast<LegacyThreadStartMember>(&typed::Threads::start)), LegacyThreadStartMember>);
    static_assert(std::same_as<decltype(static_cast<LegacyThreadResumeMember>(&typed::Threads::resume)), LegacyThreadResumeMember>);
    static_assert(std::same_as<decltype(static_cast<LegacyThreadListMember>(&typed::Threads::list)), LegacyThreadListMember>);
    static_assert(std::same_as<decltype(static_cast<LegacyThreadReadMember>(&typed::Threads::read)), LegacyThreadReadMember>);
    static_assert(
        std::same_as<decltype(static_cast<LegacyThreadReadOptionsMember>(&typed::Threads::read)), LegacyThreadReadOptionsMember>);
    static_assert(std::same_as<decltype(static_cast<LegacyTurnStartMember>(&typed::Turns::start)), LegacyTurnStartMember>);
    static_assert(std::same_as<decltype(static_cast<LegacyTurnInterruptMember>(&typed::Turns::interrupt)), LegacyTurnInterruptMember>);

    static_assert(requires(typed::Threads& threads,
                           typed::ThreadStartOptions& options,
                           typed::Threads::ThreadResultHandler& typedHandler,
                           GenericResultHandler& genericHandler) {
        { threads.start({}, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        {
            threads.start({.cwd = "/legacy-start", .sandboxMode = typed::SandboxMode::workspaceWrite(), .ephemeral = true},
                          GenericResultHandler{})
        } -> std::same_as<typed::Threads::Submission>;
        { threads.start(options, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        { threads.start(std::move(options), GenericResultHandler{}) } -> std::same_as<typed::Threads::Submission>;
        { threads.start(options, typedHandler) } -> std::same_as<typed::Threads::Submission>;
        { threads.start(std::move(options), std::move(typedHandler)) } -> std::same_as<typed::Threads::Submission>;
    });

    static_assert(requires(typed::Threads& threads,
                           typed::ThreadResumeOptions& options,
                           typed::Threads::ThreadResultHandler& typedHandler,
                           GenericResultHandler& genericHandler) {
        {
            threads.resume({"legacy-thread"},
                           {.cwd = "/legacy-resume", .sandboxMode = typed::SandboxMode::readOnly()},
                           GenericResultHandler{})
        } -> std::same_as<typed::Threads::Submission>;
        { threads.resume({"legacy-thread"}, options, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        {
            threads.resume({"legacy-thread"}, std::move(options), GenericResultHandler{})
        } -> std::same_as<typed::Threads::Submission>;
        { threads.resume({"legacy-thread"}, options, typedHandler) } -> std::same_as<typed::Threads::Submission>;
        {
            threads.resume({"legacy-thread"}, std::move(options), std::move(typedHandler))
        } -> std::same_as<typed::Threads::Submission>;
    });

    static_assert(requires(typed::Threads& threads,
                           typed::ThreadListOptions& options,
                           typed::Threads::ThreadListResultHandler& typedHandler,
                           GenericResultHandler& genericHandler) {
        { threads.list({}, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        {
            threads.list({.cursor = "legacy-cursor", .limit = 2, .archived = false, .searchTerm = "legacy"},
                         GenericResultHandler{})
        } -> std::same_as<typed::Threads::Submission>;
        { threads.list(options, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        { threads.list(std::move(options), GenericResultHandler{}) } -> std::same_as<typed::Threads::Submission>;
        { threads.list(options, typedHandler) } -> std::same_as<typed::Threads::Submission>;
        { threads.list(std::move(options), std::move(typedHandler)) } -> std::same_as<typed::Threads::Submission>;
    });

    static_assert(requires(typed::Threads& threads,
                           typed::ThreadReadOptions& options,
                           typed::Threads::ThreadResultHandler& typedHandler,
                           GenericResultHandler& genericHandler) {
        { threads.read({"legacy-thread"}, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        {
            threads.read({"legacy-thread"}, {.includeTurns = true}, GenericResultHandler{})
        } -> std::same_as<typed::Threads::Submission>;
        { threads.read({"legacy-thread"}, options, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        {
            threads.read({"legacy-thread"}, std::move(options), GenericResultHandler{})
        } -> std::same_as<typed::Threads::Submission>;
        { threads.read({"legacy-thread"}, options, typedHandler) } -> std::same_as<typed::Threads::Submission>;
        {
            threads.read({"legacy-thread"}, std::move(options), std::move(typedHandler))
        } -> std::same_as<typed::Threads::Submission>;
    });

    static_assert(requires(typed::Turns& turns,
                           std::vector<typed::TurnInput>& input,
                           typed::TurnStartOptions& options,
                           typed::Turns::TurnResultHandler& typedHandler,
                           GenericResultHandler& genericHandler) {
        {
            turns.start({"legacy-thread"},
                        {},
                        {.cwd = "/legacy-turn", .approvalPolicy = typed::ApprovalPolicy::onRequest()},
                        GenericResultHandler{})
        } -> std::same_as<typed::Turns::Submission>;
        { turns.start({"legacy-thread"}, input, options, genericHandler) } -> std::same_as<typed::Turns::Submission>;
        {
            turns.start({"legacy-thread"}, std::move(input), std::move(options), GenericResultHandler{})
        } -> std::same_as<typed::Turns::Submission>;
        { turns.start({"legacy-thread"}, input, options, typedHandler) } -> std::same_as<typed::Turns::Submission>;
        {
            turns.start({"legacy-thread"}, std::move(input), std::move(options), std::move(typedHandler))
        } -> std::same_as<typed::Turns::Submission>;
    });

    static_assert(requires(typed::Turns& turns,
                           typed::Turns::InterruptResultHandler& typedHandler,
                           GenericResultHandler& genericHandler) {
        {
            turns.interrupt({"legacy-thread"}, {"legacy-turn"}, genericHandler)
        } -> std::same_as<typed::Turns::Submission>;
        {
            turns.interrupt({"legacy-thread"}, {"legacy-turn"}, GenericResultHandler{})
        } -> std::same_as<typed::Turns::Submission>;
        {
            turns.interrupt({"legacy-thread"}, {"legacy-turn"}, typedHandler)
        } -> std::same_as<typed::Turns::Submission>;
        {
            turns.interrupt({"legacy-thread"}, {"legacy-turn"}, std::move(typedHandler))
        } -> std::same_as<typed::Turns::Submission>;
    });

    static_assert(requires(typed::Threads& threads,
                           typed::ThreadStartParams& start,
                           typed::ThreadResumeParams& resume,
                           typed::ThreadListParams& list,
                           typed::ThreadReadParams& read,
                           typed::Threads::ThreadStartResultHandler& startHandler,
                           typed::Threads::ThreadResumeResultHandler& resumeHandler,
                           typed::Threads::ThreadListResultHandler& listHandler,
                           typed::Threads::ThreadReadResultHandler& readHandler,
                           GenericResultHandler& genericHandler) {
        { threads.start(start, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        { threads.start(std::move(start), std::move(startHandler)) } -> std::same_as<typed::Threads::Submission>;
        { threads.resume(resume, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        { threads.resume(std::move(resume), std::move(resumeHandler)) } -> std::same_as<typed::Threads::Submission>;
        { threads.list(list, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        { threads.list(std::move(list), std::move(listHandler)) } -> std::same_as<typed::Threads::Submission>;
        { threads.read(read, genericHandler) } -> std::same_as<typed::Threads::Submission>;
        { threads.read(std::move(read), std::move(readHandler)) } -> std::same_as<typed::Threads::Submission>;
    });

    static_assert(requires(typed::Turns& turns,
                           typed::TurnStartParams& start,
                           typed::TurnInterruptParams& interrupt,
                           typed::Turns::TurnStartResultHandler& startHandler,
                           typed::Turns::UnitResultHandler& interruptHandler,
                           GenericResultHandler& genericHandler) {
        { turns.start(start, genericHandler) } -> std::same_as<typed::Turns::Submission>;
        { turns.start(std::move(start), std::move(startHandler)) } -> std::same_as<typed::Turns::Submission>;
        { turns.interrupt(interrupt, genericHandler) } -> std::same_as<typed::Turns::Submission>;
        {
            turns.interrupt(std::move(interrupt), std::move(interruptHandler))
        } -> std::same_as<typed::Turns::Submission>;
    });

    class InertTransport final : public codex::detail::Transport {
    public:
        void setCallbacks(codex::detail::TransportCallbacks) override {
        }

        void start() override {
        }

        bool send(std::string) override {
            return false;
        }

        void stop() override {
        }
    };

    class TestClient final : public codex::AppServerClient {
    public:
        TestClient()
            : AppServerClient(std::make_unique<InertTransport>(), {"typed_facade_test", "Typed Facade Test", "1"}) {
        }
    };
} // namespace

int main() {
    static_assert(!std::is_copy_constructible_v<codex::typed::Client>);
    static_assert(!std::is_move_constructible_v<codex::typed::Client>);
    static_assert(sizeof(codex::typed::Client) == sizeof(void*));
    static_assert(sizeof(codex::AppServerClient) == 2 * sizeof(void*));
    static_assert(std::is_same_v<decltype(std::declval<codex::AppServerClient&>().typed()), codex::typed::Client&>);
    static_assert(std::is_same_v<decltype(std::declval<const codex::AppServerClient&>().typed()), const codex::typed::Client&>);
    static_assert(!HasDirectAccountsAccessor<codex::AppServerClient>);
    static_assert(!HasDirectModelsAccessor<codex::AppServerClient>);

    tests::support::TestResult result;
    TestClient client;
    const codex::AppServerClient& constClient = client;

    result.expectTrue(&client.typed() == &client.typed(), "non-const typed() returns one stable grouped facade");
    result.expectTrue(&constClient.typed() == &client.typed(), "const typed() returns the same grouped facade");
    result.expectTrue(&client.typed().accounts() == &constClient.typed().accounts(),
                      "accounts() returns the same facade backed by the grouped client's one RawProtocol");
    result.expectTrue(&client.typed().models() == &constClient.typed().models(),
                      "models() returns the same facade backed by the grouped client's one RawProtocol");

    result.expectTrue(&client.threads() == &client.typed().threads(),
                      "deprecated threads() forwards to the grouped facade object");
    result.expectTrue(&client.turns() == &client.typed().turns(),
                      "deprecated turns() forwards to the grouped facade object");
    result.expectTrue(&client.events() == &client.typed().events(),
                      "deprecated events() forwards to the grouped facade object");
    result.expectTrue(&client.requests() == &client.typed().requests(),
                      "deprecated requests() forwards to the grouped facade object");

    result.expectTrue(&constClient.threads() == &constClient.typed().threads(),
                      "deprecated const threads() forwards to the grouped facade object");
    result.expectTrue(&constClient.turns() == &constClient.typed().turns(),
                      "deprecated const turns() forwards to the grouped facade object");
    result.expectTrue(&constClient.events() == &constClient.typed().events(),
                      "deprecated const events() forwards to the grouped facade object");
    result.expectTrue(&constClient.requests() == &constClient.typed().requests(),
                      "deprecated const requests() forwards to the grouped facade object");

    return result.processResult();
}
