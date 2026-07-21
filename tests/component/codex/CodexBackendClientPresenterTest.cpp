/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/frontend/Messages.h"
#include "apps/codex-backend-client/CommandDrainController.h"
#include "apps/codex-backend-client/Presenter.h"
#include "support/TestResult.h"

#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace {
    namespace client = apps::codex_backend_client;
    namespace frontend = ai::openai::codex::frontend;

    frontend::ServerMessage success(std::string requestId, frontend::Json result) {
        return frontend::ServerMessage{frontend::Response::success(std::move(requestId), std::move(result))};
    }

    frontend::ServerMessage failure(std::string requestId, std::string message) {
        return frontend::ServerMessage{frontend::Response::failure(
            std::move(requestId),
            frontend::CommandError{frontend::ErrorCode::InvalidCommand, std::move(message), std::nullopt, frontend::Json::object()})};
    }

    bool contains(const std::string& value, const std::string& expected) {
        return value.find(expected) != std::string::npos;
    }

    void testThreadSummaries(tests::support::TestResult& result) {
        std::ostringstream output;
        std::ostringstream diagnostics;
        client::Presenter presenter(client::OutputMode::Human, output, diagnostics);

        presenter.present(success("start-1", frontend::Json{{"thread", {{"id", "thread-created"}, {"turns", frontend::Json::array()}}}}),
                          client::ResponsePresentation{client::ResponsePresentation::Kind::ThreadStart, "thread-created"});
        presenter.present(success("resume-1", frontend::Json{{"threadId", "thread-persisted"}}),
                          client::ResponsePresentation{client::ResponsePresentation::Kind::ThreadResume, "thread-persisted"});

        result.expectTrue(output.str() == "thread started id=thread-created\nthread resumed id=thread-persisted\n",
                          "human presentation identifies started and resumed threads without dumping their snapshots");
        result.expectTrue(diagnostics.str().empty(), "successful human thread summaries produce no diagnostics");
    }

    void testNewSummaries(tests::support::TestResult& result) {
        std::ostringstream output;
        std::ostringstream diagnostics;
        client::Presenter presenter(client::OutputMode::Human, output, diagnostics);

        presenter.present(success("new-start", frontend::Json{{"thread", {{"id", "thread-new"}}}}),
                          client::ResponsePresentation{client::ResponsePresentation::Kind::NewThreadStart, "thread-new"});
        presenter.present(success("new-turn", frontend::Json{{"turn", {{"id", "turn-first"}, {"items", frontend::Json::array()}}}}),
                          client::ResponsePresentation{client::ResponsePresentation::Kind::NewTurnStart, "thread-new"});

        result.expectTrue(output.str() == "thread created id=thread-new\ninitial turn submitted thread=thread-new turn=turn-first\n",
                          "human presentation reports both successful stages of the client-side new operation");
        result.expectTrue(diagnostics.str().empty(), "successful new summaries produce no diagnostics");
    }

    void testFailureSummariesAreBounded(tests::support::TestResult& result) {
        std::ostringstream output;
        std::ostringstream diagnostics;
        client::Presenter presenter(client::OutputMode::Human, output, diagnostics);

        presenter.present(failure("start-failed", "controller ownership is required"),
                          client::ResponsePresentation{client::ResponsePresentation::Kind::ThreadStart, std::nullopt});
        presenter.present(failure("new-turn-failed", std::string(2048, 'x')),
                          client::ResponsePresentation{client::ResponsePresentation::Kind::NewTurnStart, "thread-survives"});

        const std::string rendered = output.str();
        result.expectTrue(contains(rendered, "thread start failed: invalid_command: controller ownership is required"),
                          "human presentation gives a useful thread-start failure summary");
        result.expectTrue(contains(rendered, "thread created id=thread-survives, but initial turn failed: invalid_command:"),
                          "partial new failure preserves the successfully created thread ID");
        result.expectTrue(rendered.size() < 800, "human failure summaries stay bounded even when the backend error message is very large");
        result.expectTrue(diagnostics.str().empty(), "protocol response failures remain protocol output rather than local diagnostics");
    }

    void testJsonPreservesOriginalMessages(tests::support::TestResult& result) {
        std::ostringstream output;
        std::ostringstream diagnostics;
        client::Presenter presenter(client::OutputMode::Json, output, diagnostics);
        const frontend::ServerMessage message =
            success("json-new-start", frontend::Json{{"thread", {{"id", "thread-json"}, {"preview", std::string(5000, 'p')}}}});

        presenter.present(message, client::ResponsePresentation{client::ResponsePresentation::Kind::NewThreadStart, "thread-json"});
        const auto encoded = frontend::Codec::serializeServer(message);

        result.expectTrue(encoded.hasValue() && output.str() == encoded.value() + "\n",
                          "JSON mode emits the complete original Frontend Protocol response despite local new correlation");
        result.expectTrue(diagnostics.str().empty(), "JSON response presentation adds no synthetic new diagnostic or protocol message");
    }
} // namespace

int main() {
    tests::support::TestResult result;

    testThreadSummaries(result);
    testNewSummaries(result);
    testFailureSummariesAreBounded(result);
    testJsonPreservesOriginalMessages(result);

    return result.processResult();
}
