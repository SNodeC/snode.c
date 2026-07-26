/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/Transport.h"
#include "ai/openai/codex/typed/Client.h"
#include "ai/openai/codex/typed/PermissionProfiles.h"
#include "ai/openai/codex/typed/ServerRequests.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    constexpr std::size_t InitialRequestCount = 5;

    bool writeFully(int descriptor, std::string_view bytes) {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const ssize_t written = ::write(descriptor, bytes.data() + offset, bytes.size() - offset);
            if (written > 0) {
                offset += static_cast<std::size_t>(written);
            } else if (written < 0 && errno == EINTR) {
                continue;
            } else {
                return false;
            }
        }
        return true;
    }

    std::optional<std::string> readLine(int descriptor) {
        std::string line;
        for (;;) {
            char byte = '\0';
            const ssize_t received = ::read(descriptor, &byte, 1);
            if (received == 1) {
                line.push_back(byte);
                if (byte == '\n') {
                    return line;
                }
            } else if (received < 0 && errno == EINTR) {
                continue;
            } else {
                return std::nullopt;
            }
        }
    }

    codex::Json initializeResult() {
        return {
            {"codexHome", "/tmp/codex-a1-3-approval-wire"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-a1-3-approval-wire/1"},
        };
    }

    struct OutboundRecord {
        std::string line;
        codex::Json envelope;
    };

    struct UnixTransportState {
        detail::TransportCallbacks callbacks;
        std::vector<detail::TransportCallbacks> callbackGenerations;
        std::vector<OutboundRecord> outbound;
        int clientDescriptor = -1;
        int serverDescriptor = -1;
        bool running = false;

        ~UnixTransportState() {
            if (clientDescriptor >= 0) {
                ::close(clientDescriptor);
            }
            if (serverDescriptor >= 0) {
                ::close(serverDescriptor);
            }
        }

        bool open() {
            int descriptors[2]{-1, -1};
            if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, descriptors) != 0) {
                return false;
            }
            clientDescriptor = descriptors[0];
            serverDescriptor = descriptors[1];
            return true;
        }

        bool injectWith(const detail::TransportCallbacks& target, const codex::Json& envelope) {
            const std::string line = envelope.dump() + "\n";
            if (!writeFully(serverDescriptor, line)) {
                return false;
            }
            const std::optional<std::string> received = readLine(clientDescriptor);
            if (!received || *received != line || !target.onMessage) {
                return false;
            }
            target.onMessage(received->substr(0, received->size() - 1));
            return true;
        }

        bool injectCurrent(const codex::Json& envelope) {
            return injectWith(callbacks, envelope);
        }

        bool send(std::string message) {
            const std::string line = std::move(message) + "\n";
            if (!writeFully(clientDescriptor, line)) {
                return false;
            }
            const std::optional<std::string> received = readLine(serverDescriptor);
            if (!received || *received != line) {
                return false;
            }
            codex::Json envelope = codex::Json::parse(received->begin(), received->end() - 1, nullptr, false);
            if (envelope.is_discarded()) {
                return false;
            }
            outbound.push_back({*received, envelope});

            const auto method = envelope.find("method");
            if (method != envelope.end() && method->is_string() && *method == "initialize") {
                const auto id = envelope.find("id");
                return id != envelope.end() && injectCurrent({{"id", *id}, {"result", initializeResult()}});
            }
            if (method != envelope.end() && method->is_string() && *method == "permissionProfile/list") {
                const auto id = envelope.find("id");
                return id != envelope.end() &&
                       injectCurrent({{"id", *id},
                                      {"result",
                                       {{"data",
                                         codex::Json::array({{{"allowed", true}, {"description", nullptr}, {"id", "synthetic-profile-a"}},
                                                             {{"allowed", false},
                                                              {"description", "synthetic profile description"},
                                                              {"id", "synthetic-profile-b"}}})},
                                        {"nextCursor", "synthetic-next"},
                                        {"futureResponseField", true}}}});
            }
            return true;
        }
    };

    class UnixTranscriptTransport final : public detail::Transport {
    public:
        explicit UnixTranscriptTransport(std::shared_ptr<UnixTransportState> state)
            : state(std::move(state)) {
        }

        void setCallbacks(detail::TransportCallbacks callbacks) override {
            state->callbacks = std::move(callbacks);
            if (state->callbacks.onStarted || state->callbacks.onMessage || state->callbacks.onDiagnostic || state->callbacks.onError ||
                state->callbacks.onExited) {
                state->callbackGenerations.push_back(state->callbacks);
            }
        }

        void start() override {
            state->running = true;
            if (state->callbacks.onStarted) {
                state->callbacks.onStarted();
            }
        }

        bool send(std::string message) override {
            return state->send(std::move(message));
        }

        void stop() override {
            if (!std::exchange(state->running, false)) {
                return;
            }
            if (state->callbacks.onExited) {
                state->callbacks.onExited(detail::ProcessExit{true, 0, false, 0});
            }
        }

    private:
        std::shared_ptr<UnixTransportState> state;
    };

    class TestClient final : public codex::AppServerClient {
    public:
        explicit TestClient(const std::shared_ptr<UnixTransportState>& state)
            : AppServerClient(std::make_unique<UnixTranscriptTransport>(state),
                              {"codex_a1_3_approval_wire_test", "Codex A1.3 Approval Wire Test", "1"}) {
        }
    };

    codex::Json applyPatchParams() {
        return {
            {"callId", "call-patch"},
            {"conversationId", "thread-patch"},
            {"fileChanges",
             {{"/synthetic/add", {{"type", "add"}, {"content", "synthetic-add"}}},
              {"/synthetic/delete", {{"type", "delete"}, {"content", "synthetic-delete"}}},
              {"/synthetic/update", {{"type", "update"}, {"move_path", nullptr}, {"unified_diff", "synthetic-diff"}}}}},
            {"grantRoot", nullptr},
            {"reason", "synthetic patch reason"},
        };
    }

    codex::Json execCommandParams() {
        return {
            {"approvalId", nullptr},
            {"callId", "call-exec"},
            {"command", codex::Json::array({"synthetic-command", "argument with spaces", ""})},
            {"conversationId", "thread-exec"},
            {"cwd", "./synthetic/../exec-cwd"},
            {"parsedCmd",
             codex::Json::array({{{"type", "read"}, {"cmd", "read"}, {"name", "synthetic-name"}, {"path", "/synthetic/read"}},
                                 {{"type", "list_files"}, {"cmd", "list"}, {"path", nullptr}},
                                 {{"type", "search"}, {"cmd", "search"}, {"path", "/synthetic/search"}, {"query", "needle"}},
                                 {{"type", "unknown"}, {"cmd", "synthetic-unknown"}}})},
            {"reason", nullptr},
        };
    }

    codex::Json commandRequestParams() {
        return {
            {"approvalId", "approval-command"},
            {"command", "synthetic command description"},
            {"commandActions", codex::Json::array({{{"type", "unknown"}, {"command", "synthetic-action"}}})},
            {"cwd", "/synthetic/command-cwd"},
            {"environmentId", nullptr},
            {"itemId", "item-command"},
            {"networkApprovalContext", {{"host", "synthetic.invalid"}, {"protocol", "https"}}},
            {"proposedExecpolicyAmendment", codex::Json::array({"synthetic-policy"})},
            {"proposedNetworkPolicyAmendments", codex::Json::array({{{"action", "allow"}, {"host", "synthetic.invalid"}}})},
            {"reason", "synthetic command reason"},
            {"startedAtMs", 101},
            {"threadId", "thread-command"},
            {"turnId", "turn-command"},
        };
    }

    codex::Json fileChangeRequestParams(std::string itemId = "item-file") {
        return {
            {"grantRoot", nullptr},
            {"itemId", std::move(itemId)},
            {"reason", "synthetic file reason"},
            {"startedAtMs", 102},
            {"threadId", "thread-file"},
            {"turnId", "turn-file"},
        };
    }

    codex::Json permissionsRequestParams() {
        return {
            {"cwd", "/synthetic/permission-cwd"},
            {"environmentId", nullptr},
            {"itemId", "item-permission"},
            {"permissions",
             {{"fileSystem",
               {{"entries",
                 codex::Json::array({{{"access", "write"}, {"path", {{"type", "special"}, {"value", {{"kind", "slash_tmp"}}}}}}})},
                {"globScanMaxDepth", 1},
                {"read", nullptr},
                {"write", codex::Json::array({"/synthetic/write"})}}},
              {"network", {{"enabled", true}}}}},
            {"reason", "synthetic permission reason"},
            {"startedAtMs", 103},
            {"threadId", "thread-permission"},
            {"turnId", "turn-permission"},
        };
    }

    codex::Json requestEnvelope(codex::Json id, std::string method, codex::Json params, std::string extension) {
        return {
            {"id", std::move(id)},
            {"method", std::move(method)},
            {"params", std::move(params)},
            {"syntheticEnvelopeExtension", std::move(extension)},
        };
    }

    typed::PermissionsRequestApprovalResponse permissionResponse() {
        typed::GrantedPermissionProfile permissions;
        typed::AdditionalFileSystemPermissions fileSystem;
        fileSystem.entries = typed::OptionalNullable<std::vector<typed::FileSystemSandboxEntry>>::withValue(
            {{.access = typed::FileSystemAccessMode::write(),
              .path = typed::SpecialFileSystemPath{.value = typed::SlashTmpFileSystemSpecialPath{},
                                                   .raw = codex::Json::object(),
                                                   .diagnostics = {}},
              .raw = codex::Json::object(),
              .diagnostics = {}}});
        fileSystem.globScanMaxDepth = typed::OptionalNullable<std::uint64_t>::withValue(1);
        fileSystem.read = typed::OptionalNullable<std::vector<std::string>>::explicitNull();
        fileSystem.write = typed::OptionalNullable<std::vector<std::string>>::withValue({"/synthetic/write"});
        permissions.fileSystem = typed::OptionalNullable<typed::AdditionalFileSystemPermissions>::withValue(std::move(fileSystem));
        permissions.network = typed::OptionalNullable<typed::AdditionalNetworkPermissions>::withValue(
            {.enabled = typed::OptionalNullable<bool>::withValue(true), .raw = codex::Json::object(), .diagnostics = {}});
        return {std::move(permissions), typed::PermissionGrantScope::session(), typed::OptionalNullable<bool>::withValue(false)};
    }

    class ApprovalWireRunner {
    public:
        explicit ApprovalWireRunner(tests::support::TestResult& result)
            : result(result)
            , state(std::make_shared<UnixTransportState>())
            , socketReady(state->open())
            , client(std::make_unique<TestClient>(state)) {
        }

        void start() {
            expect(socketReady, "A1.3 approval wire test opens its AF_UNIX socketpair");
            if (!socketReady) {
                finished = true;
                core::SNodeC::stop();
                return;
            }

            const auto localRejection =
                client->typed().permissionProfiles().list({}, [this](const typed::OperationResult<typed::PermissionProfileListResponse>&) {
                    ++unexpectedProfileCallbacks;
                });
            expect(!localRejection && !localRejection.id && localRejection.error &&
                       localRejection.error->category == codex::Error::Category::InvalidState && unexpectedProfileCallbacks == 0 &&
                       state->outbound.empty(),
                   "permissionProfile/list rejects synchronously before RawProtocol is ready");

            client->typed().requests().setOnRequest([this](const typed::TypedServerRequest& request) {
                insideTypedCallback = true;
                handleRequest(request);
                insideTypedCallback = false;
            });
            client->raw().setOnServerRequest([this](const codex::ServerRequest& request) {
                ++rawRequestCount;
                rawMethods.insert(request.method);
            });
            client->setOnStateChanged([this](const codex::StateChange& change) {
                handleState(change);
            });
            client->start();
        }

        [[nodiscard]] bool isFinished() const noexcept {
            return finished;
        }

    private:
        enum class Phase { Initial, Disconnecting, Reconnected };

        void expect(bool condition, std::string message) {
            result.expectTrue(condition, std::move(message));
        }

        typed::Requests::SendResult respondForLifecycle(const typed::ApplyPatchApprovalRequest& request) {
            return client->typed().requests().respond(request, typed::ApplyPatchApprovalResponse{typed::DeniedReviewDecision{}});
        }

        typed::Requests::SendResult respondForLifecycle(const typed::ExecCommandApprovalRequest& request) {
            return client->typed().requests().respond(request, typed::ExecCommandApprovalResponse{typed::TimedOutReviewDecision{}});
        }

        typed::Requests::SendResult respondForLifecycle(const typed::CommandApprovalRequest& request) {
            return client->typed().requests().respond(
                request, typed::CommandExecutionRequestApprovalResponse{typed::DeclineCommandExecutionApprovalDecision{}});
        }

        typed::Requests::SendResult respondForLifecycle(const typed::FileChangeApprovalRequest& request) {
            return client->typed().requests().respond(
                request, typed::FileChangeRequestApprovalResponse{typed::FileChangeApprovalDecision::cancel()});
        }

        typed::Requests::SendResult respondForLifecycle(const typed::PermissionsApprovalRequest& request) {
            return client->typed().requests().respond(request, permissionResponse());
        }

        typed::Requests::SendResult respondForLifecycle(const typed::TypedServerRequest& request) {
            if (const auto* typedRequest = std::get_if<typed::ApplyPatchApprovalRequest>(&request)) {
                return respondForLifecycle(*typedRequest);
            }
            if (const auto* typedRequest = std::get_if<typed::ExecCommandApprovalRequest>(&request)) {
                return respondForLifecycle(*typedRequest);
            }
            if (const auto* typedRequest = std::get_if<typed::CommandApprovalRequest>(&request)) {
                return respondForLifecycle(*typedRequest);
            }
            if (const auto* typedRequest = std::get_if<typed::FileChangeApprovalRequest>(&request)) {
                return respondForLifecycle(*typedRequest);
            }
            if (const auto* typedRequest = std::get_if<typed::PermissionsApprovalRequest>(&request)) {
                return respondForLifecycle(*typedRequest);
            }
            return {false, codex::Error{codex::Error::Category::Protocol, EINVAL, "non-A1.3 lifecycle request in test corpus"}};
        }

        void handleState(const codex::StateChange& change) {
            if (change.current == codex::State::Ready) {
                ++readyCount;
                if (readyCount == 1) {
                    core::EventReceiver::atNextTick([this]() {
                        beginPermissionProfiles();
                    });
                } else if (readyCount == 2) {
                    core::EventReceiver::atNextTick([this]() {
                        probeStaleTransportGeneration();
                    });
                }
                return;
            }
            if (change.current == codex::State::Failed) {
                expect(false, "A1.3 approval wire lifecycle must not fail");
                client->stop();
                return;
            }
            if (change.current != codex::State::Stopped) {
                return;
            }

            ++stoppedCount;
            if (stoppedCount == 1) {
                expect(oldGenerationRequests.size() == InitialRequestCount, "disconnect scenario retains all five old typed occurrences");
                std::size_t invalidated = 0;
                for (const typed::TypedServerRequest& request : oldGenerationRequests) {
                    const auto stale = respondForLifecycle(request);
                    if (!stale && stale.error && stale.error->category == codex::Error::Category::InvalidState) {
                        ++invalidated;
                    }
                }
                expect(invalidated == InitialRequestCount, "disconnect invalidates every A1.3 approval occurrence");
                core::EventReceiver::atNextTick([this]() {
                    client->start();
                });
                return;
            }

            finished = true;
            core::SNodeC::stop();
        }

        void beginPermissionProfiles() {
            const std::size_t before = state->outbound.size();
            const typed::PermissionProfileListParams params{
                typed::OptionalNullable<std::string>::explicitNull(),
                typed::OptionalNullable<std::string>::withValue("./synthetic/../profile-cwd"),
                typed::OptionalNullable<std::uint32_t>::withValue(std::numeric_limits<std::uint32_t>::max())};
            const auto submission = client->typed().permissionProfiles().list(
                params, [this](const typed::OperationResult<typed::PermissionProfileListResponse>& operation) {
                    expect(operation && operation.value->data.size() == 2 && operation.value->data[0].allowed &&
                               operation.value->data[0].description.isNull() && !operation.value->data[1].allowed &&
                               operation.value->data[1].description.value == std::optional<std::string>{"synthetic profile description"} &&
                               operation.value->nextCursor.value == std::optional<std::string>{"synthetic-next"} &&
                               operation.raw.value("futureResponseField", false),
                           "permissionProfile/list decodes ordered data, nullability, and raw extensions");
                    core::EventReceiver::atNextTick([this]() {
                        beginInitialRequests();
                    });
                });
            expect(submission && submission.id && !submission.error, "permissionProfile/list is accepted by the existing RawProtocol");
            if (!submission || !submission.id || state->outbound.size() != before + 1) {
                client->stop();
                return;
            }
            const codex::Json expected{
                {"id", submission.id->value()},
                {"method", "permissionProfile/list"},
                {"params",
                 {{"cursor", nullptr}, {"cwd", "./synthetic/../profile-cwd"}, {"limit", std::numeric_limits<std::uint32_t>::max()}}},
            };
            expect(state->outbound.back().envelope == expected && state->outbound.back().line == expected.dump() + "\n",
                   "permissionProfile/list emits exact JSON-RPC/JSONL bytes");
        }

        void beginInitialRequests() {
            phase = Phase::Initial;
            const std::size_t before = state->outbound.size();
            serverRequestBaseline = before;
            const std::vector<codex::Json> requests{
                requestEnvelope(101, "applyPatchApproval", applyPatchParams(), "patch"),
                requestEnvelope("request-exec", "execCommandApproval", execCommandParams(), "exec"),
                requestEnvelope(103, "item/commandExecution/requestApproval", commandRequestParams(), "command"),
                requestEnvelope("request-file", "item/fileChange/requestApproval", fileChangeRequestParams(), "file"),
                requestEnvelope(105, "item/permissions/requestApproval", permissionsRequestParams(), "permissions"),
            };
            std::size_t injected = 0;
            for (const codex::Json& request : requests) {
                if (state->injectCurrent(request)) {
                    ++injected;
                }
            }
            expect(injected == InitialRequestCount, "all five approval requests cross the real socket");
            expect(state->outbound.size() == before, "incoming approvals never trigger an automatic approval or denial");
        }

        void handleRequest(const typed::TypedServerRequest& request) {
            ++typedRequestCount;
            if (phase == Phase::Initial) {
                handleInitialRequest(request);
                return;
            }
            if (phase == Phase::Disconnecting) {
                handleDisconnectingRequest(request);
                return;
            }
            handleReconnectedRequest(request);
        }

        void handleInitialRequest(const typed::TypedServerRequest& request) {
            bool decoded = false;
            if (const auto* typedRequest = std::get_if<typed::ApplyPatchApprovalRequest>(&request)) {
                patchRequest = *typedRequest;
                decoded = typedRequest->params.callId.value == "call-patch" && typedRequest->params.fileChanges.size() == 3 &&
                          typedRequest->raw.value("syntheticEnvelopeExtension", "") == "patch";
            } else if (const auto* typedRequest = std::get_if<typed::ExecCommandApprovalRequest>(&request)) {
                execRequest = *typedRequest;
                decoded = typedRequest->params.command == std::vector<std::string>({"synthetic-command", "argument with spaces", ""}) &&
                          typedRequest->params.parsedCommand.size() == 4 &&
                          typedRequest->raw.value("syntheticEnvelopeExtension", "") == "exec";
            } else if (const auto* typedRequest = std::get_if<typed::CommandApprovalRequest>(&request)) {
                commandRequest = *typedRequest;
                decoded = typedRequest->canonicalParams.environmentId.isNull() &&
                          typedRequest->canonicalParams.networkApprovalContext.value &&
                          typedRequest->canonicalParams.proposedNetworkPolicyAmendments.value &&
                          typedRequest->canonicalParams.startedAtMs == 101 &&
                          typedRequest->raw.value("syntheticEnvelopeExtension", "") == "command";
            } else if (const auto* typedRequest = std::get_if<typed::FileChangeApprovalRequest>(&request)) {
                fileRequest = *typedRequest;
                decoded = typedRequest->canonicalParams.grantRoot.isNull() && typedRequest->canonicalParams.startedAtMs == 102 &&
                          typedRequest->raw.value("syntheticEnvelopeExtension", "") == "file";
            } else if (const auto* typedRequest = std::get_if<typed::PermissionsApprovalRequest>(&request)) {
                permissionsRequest = *typedRequest;
                decoded = typedRequest->params.environmentId.isNull() && typedRequest->params.permissions.fileSystem.value &&
                          typedRequest->params.permissions.fileSystem.value->entries.value &&
                          typedRequest->params.raw == permissionsRequestParams() &&
                          typedRequest->raw.value("syntheticEnvelopeExtension", "") == "permissions";
            }

            expect(decoded, "each A1.3 approval request decodes as its exact typed root");
            if (typedRequestCount == 1) {
                callbackThrowObserved = true;
                throw std::runtime_error("intentional A1.3 approval callback failure");
            }
            if (typedRequestCount == InitialRequestCount) {
                respondInitialOutOfOrder();
            }
        }

        void respondInitialOutOfOrder() {
            expect(patchRequest && execRequest && commandRequest && fileRequest && permissionsRequest,
                   "all five concrete request types remain pending concurrently");
            if (!patchRequest || !execRequest || !commandRequest || !fileRequest || !permissionsRequest) {
                client->stop();
                return;
            }

            const std::set<std::uint64_t> tokens{
                patchRequest->requestToken.value(),
                execRequest->requestToken.value(),
                commandRequest->requestToken.value(),
                fileRequest->requestToken.value(),
                permissionsRequest->requestToken.value(),
            };
            expect(tokens.size() == InitialRequestCount && !tokens.contains(0),
                   "every concurrent request receives a distinct nonzero occurrence token");
            expect(state->outbound.size() == serverRequestBaseline, "all five requests stay unanswered until the application responds");

            const std::size_t beforeWrongType = state->outbound.size();
            typed::ApplyPatchApprovalRequest wrongPatch = *patchRequest;
            wrongPatch.requestId = execRequest->requestId;
            wrongPatch.requestToken = execRequest->requestToken;
            typed::ExecCommandApprovalRequest wrongExec = *execRequest;
            wrongExec.requestId = commandRequest->requestId;
            wrongExec.requestToken = commandRequest->requestToken;
            typed::CommandApprovalRequest wrongCommand = *commandRequest;
            wrongCommand.requestId = fileRequest->requestId;
            wrongCommand.requestToken = fileRequest->requestToken;
            typed::FileChangeApprovalRequest wrongFile = *fileRequest;
            wrongFile.requestId = permissionsRequest->requestId;
            wrongFile.requestToken = permissionsRequest->requestToken;
            typed::PermissionsApprovalRequest wrongPermissions = *permissionsRequest;
            wrongPermissions.requestId = patchRequest->requestId;
            wrongPermissions.requestToken = patchRequest->requestToken;
            const std::vector<typed::TypedServerRequest> wrongTypes{wrongPatch, wrongExec, wrongCommand, wrongFile, wrongPermissions};
            std::size_t rejectedWrongTypes = 0;
            for (const typed::TypedServerRequest& wrongType : wrongTypes) {
                const auto rejected = respondForLifecycle(wrongType);
                if (!rejected && rejected.error && rejected.error->category == codex::Error::Category::InvalidState) {
                    ++rejectedWrongTypes;
                }
            }
            expect(rejectedWrongTypes == InitialRequestCount && state->outbound.size() == beforeWrongType,
                   "all five forged response-type associations are rejected before enqueue");

            typed::ApplyPatchApprovalRequest stalePatch = *patchRequest;
            stalePatch.requestToken = codex::ServerRequestToken{};
            typed::ExecCommandApprovalRequest staleExec = *execRequest;
            staleExec.requestToken = codex::ServerRequestToken{};
            typed::CommandApprovalRequest staleCommand = *commandRequest;
            staleCommand.requestToken = codex::ServerRequestToken{};
            typed::FileChangeApprovalRequest staleFile = *fileRequest;
            staleFile.requestToken = codex::ServerRequestToken{};
            typed::PermissionsApprovalRequest stalePermissions = *permissionsRequest;
            stalePermissions.requestToken = codex::ServerRequestToken{};
            const std::vector<typed::TypedServerRequest> staleTokens{stalePatch, staleExec, staleCommand, staleFile, stalePermissions};
            std::size_t rejectedStaleTokens = 0;
            for (const typed::TypedServerRequest& staleToken : staleTokens) {
                const auto rejected = respondForLifecycle(staleToken);
                if (!rejected && rejected.error && rejected.error->category == codex::Error::Category::InvalidState) {
                    ++rejectedStaleTokens;
                }
            }
            expect(rejectedStaleTokens == InitialRequestCount && state->outbound.size() == beforeWrongType,
                   "a stale occurrence token cannot consume any of the five pending request types");

            const std::size_t beforeInvalid = state->outbound.size();
            const auto invalid = client->typed().requests().respond(
                *commandRequest,
                typed::CommandExecutionRequestApprovalResponse{
                    typed::UnrecognizedCommandExecutionApprovalDecision{"future-decision", codex::Json{{"future", true}}, {}}});
            expect(!invalid && invalid.error && invalid.error->category == codex::Error::Category::Protocol &&
                       state->outbound.size() == beforeInvalid,
                   "typed response validation fails before enqueue and retains ownership");

            const std::size_t before = state->outbound.size();
            responseInsideCallback = insideTypedCallback;
            const auto permissions = client->typed().requests().respond(*permissionsRequest, permissionResponse());
            const auto file = client->typed().requests().respond(
                *fileRequest, typed::FileChangeRequestApprovalResponse{typed::FileChangeApprovalDecision::acceptForSession()});
            const auto command = client->typed().requests().respond(
                *commandRequest,
                typed::CommandExecutionRequestApprovalResponse{
                    typed::AcceptWithExecpolicyAmendmentCommandExecutionApprovalDecision{{"synthetic-command-policy"}}});
            const auto exec =
                client->typed().requests().respond(*execRequest,
                                                   typed::ExecCommandApprovalResponse{typed::NetworkPolicyAmendmentReviewDecision{
                                                       {.action = typed::NetworkPolicyRuleAction::deny(),
                                                        .host = "synthetic.invalid",
                                                        .raw = codex::Json::object(),
                                                        .diagnostics = {}}}});
            const auto patch = client->typed().requests().respond(
                *patchRequest,
                typed::ApplyPatchApprovalResponse{typed::ApprovedExecpolicyAmendmentReviewDecision{{"synthetic-patch-policy"}}});
            expect(permissions && file && command && exec && patch && responseInsideCallback,
                   "all five typed responses are accepted reentrantly in reverse arrival order");
            expect(state->outbound.size() == before + InitialRequestCount, "exactly one socket response is written for each request");

            const std::vector<codex::Json> expected{
                {{"id", 105},
                 {"result",
                  {{"permissions",
                    {{"fileSystem",
                      {{"entries",
                        codex::Json::array({{{"access", "write"}, {"path", {{"type", "special"}, {"value", {{"kind", "slash_tmp"}}}}}}})},
                       {"globScanMaxDepth", 1},
                       {"read", nullptr},
                       {"write", codex::Json::array({"/synthetic/write"})}}},
                     {"network", {{"enabled", true}}}}},
                   {"scope", "session"},
                   {"strictAutoReview", false}}}},
                {{"id", "request-file"}, {"result", {{"decision", "acceptForSession"}}}},
                {{"id", 103},
                 {"result",
                  {{"decision",
                    {{"acceptWithExecpolicyAmendment", {{"execpolicy_amendment", codex::Json::array({"synthetic-command-policy"})}}}}}}}},
                {{"id", "request-exec"},
                 {"result",
                  {{"decision",
                    {{"network_policy_amendment", {{"network_policy_amendment", {{"action", "deny"}, {"host", "synthetic.invalid"}}}}}}}}}},
                {{"id", 101},
                 {"result",
                  {{"decision",
                    {{"approved_execpolicy_amendment",
                      {{"proposed_execpolicy_amendment", codex::Json::array({"synthetic-patch-policy"})}}}}}}}},
            };
            std::size_t exact = 0;
            for (std::size_t index = 0; index < expected.size(); ++index) {
                const OutboundRecord& record = state->outbound[before + index];
                if (record.envelope == expected[index] && record.line == expected[index].dump() + "\n") {
                    ++exact;
                }
            }
            expect(exact == InitialRequestCount, "out-of-order responses retain exact IDs, schemas, decisions, and JSONL bytes");

            const std::vector<typed::TypedServerRequest> completed{
                *patchRequest, *execRequest, *commandRequest, *fileRequest, *permissionsRequest};
            std::size_t rejectedDuplicates = 0;
            for (const typed::TypedServerRequest& request : completed) {
                const auto duplicate = respondForLifecycle(request);
                if (!duplicate && duplicate.error && duplicate.error->category == codex::Error::Category::InvalidState) {
                    ++rejectedDuplicates;
                }
            }
            expect(rejectedDuplicates == InitialRequestCount && state->outbound.size() == before + InitialRequestCount,
                   "duplicate responses for all five request types are rejected without another write");

            expect(callbackThrowObserved, "a throwing request callback is contained before later requests respond");
            core::EventReceiver::atNextTick([this]() {
                beginDisconnectScenario();
            });
        }

        void beginDisconnectScenario() {
            expect(rawRequestCount == InitialRequestCount && rawMethods == std::set<std::string>{"applyPatchApproval",
                                                                                                 "execCommandApproval",
                                                                                                 "item/commandExecution/requestApproval",
                                                                                                 "item/fileChange/requestApproval",
                                                                                                 "item/permissions/requestApproval"},
                   "typed and raw observers coexist for all five request methods");
            phase = Phase::Disconnecting;
            const std::vector<codex::Json> requests{
                requestEnvelope("generation-patch", "applyPatchApproval", applyPatchParams(), "old-generation-patch"),
                requestEnvelope("generation-exec", "execCommandApproval", execCommandParams(), "old-generation-exec"),
                requestEnvelope(
                    "generation-command", "item/commandExecution/requestApproval", commandRequestParams(), "old-generation-command"),
                requestEnvelope("generation-file",
                                "item/fileChange/requestApproval",
                                fileChangeRequestParams("item-old-generation"),
                                "old-generation-file"),
                requestEnvelope(
                    "generation-permissions", "item/permissions/requestApproval", permissionsRequestParams(), "old-generation-permissions"),
            };
            std::size_t injected = 0;
            for (const codex::Json& request : requests) {
                if (state->injectCurrent(request)) {
                    ++injected;
                }
            }
            expect(injected == InitialRequestCount, "all five old-generation requests cross the socket");
        }

        void handleDisconnectingRequest(const typed::TypedServerRequest& request) {
            const bool expectedType = std::holds_alternative<typed::ApplyPatchApprovalRequest>(request) ||
                                      std::holds_alternative<typed::ExecCommandApprovalRequest>(request) ||
                                      std::holds_alternative<typed::CommandApprovalRequest>(request) ||
                                      std::holds_alternative<typed::FileChangeApprovalRequest>(request) ||
                                      std::holds_alternative<typed::PermissionsApprovalRequest>(request);
            expect(expectedType, "disconnect scenario decodes an A1.3 typed occurrence");
            if (expectedType) {
                oldGenerationRequests.push_back(request);
            }
            if (oldGenerationRequests.size() == InitialRequestCount) {
                core::EventReceiver::atNextTick([this]() {
                    client->stop();
                });
            }
        }

        void probeStaleTransportGeneration() {
            expect(state->callbackGenerations.size() >= 2, "reconnect installs a distinct transport callback generation");
            if (state->callbackGenerations.size() < 2) {
                client->stop();
                return;
            }
            const std::size_t before = typedRequestCount;
            expect(state->injectWith(state->callbackGenerations.front(),
                                     requestEnvelope("stale-transport-request",
                                                     "item/fileChange/requestApproval",
                                                     fileChangeRequestParams("item-stale-transport"),
                                                     "stale-transport")),
                   "stale-generation bytes cross the AF_UNIX transcript");
            core::EventReceiver::atNextTick([this, before]() {
                expect(typedRequestCount == before, "stale transport callbacks cannot resurrect a server request");
                phase = Phase::Reconnected;
                const std::vector<codex::Json> requests{
                    requestEnvelope("generation-patch", "applyPatchApproval", applyPatchParams(), "current-generation-patch"),
                    requestEnvelope("generation-exec", "execCommandApproval", execCommandParams(), "current-generation-exec"),
                    requestEnvelope("generation-command",
                                    "item/commandExecution/requestApproval",
                                    commandRequestParams(),
                                    "current-generation-command"),
                    requestEnvelope("generation-file",
                                    "item/fileChange/requestApproval",
                                    fileChangeRequestParams("item-current-generation"),
                                    "current-generation-file"),
                    requestEnvelope("generation-permissions",
                                    "item/permissions/requestApproval",
                                    permissionsRequestParams(),
                                    "current-generation-permissions"),
                };
                std::size_t injected = 0;
                for (const codex::Json& request : requests) {
                    if (state->injectCurrent(request)) {
                        ++injected;
                    }
                }
                expect(injected == InitialRequestCount, "all five current-generation requests cross the socket");
            });
        }

        void handleReconnectedRequest(const typed::TypedServerRequest& request) {
            currentGenerationRequests.push_back(request);
            if (currentGenerationRequests.size() != InitialRequestCount) {
                return;
            }

            expect(oldGenerationRequests.size() == InitialRequestCount,
                   "reconnect retains the five invalidated typed occurrences for comparison");
            if (oldGenerationRequests.size() != InitialRequestCount) {
                client->stop();
                return;
            }
            const auto token = [](const typed::TypedServerRequest& value) {
                return std::visit(
                    [](const auto& typedRequest) {
                        return typedRequest.requestToken;
                    },
                    value);
            };
            std::size_t distinctTokens = 0;
            for (std::size_t index = 0; index < InitialRequestCount; ++index) {
                distinctTokens += token(oldGenerationRequests[index]) != token(currentGenerationRequests[index]) ? 1U : 0U;
            }
            expect(distinctTokens == InitialRequestCount, "reconnect creates new occurrence tokens for all five reused request IDs");

            const std::size_t before = state->outbound.size();
            std::size_t rejectedOld = 0;
            for (const typed::TypedServerRequest& oldRequest : oldGenerationRequests) {
                const auto stale = respondForLifecycle(oldRequest);
                if (!stale && stale.error && stale.error->category == codex::Error::Category::InvalidState) {
                    ++rejectedOld;
                }
            }
            expect(rejectedOld == InitialRequestCount && state->outbound.size() == before,
                   "no old occurrence token can answer its same-ID request after reconnect");

            std::size_t acceptedCurrent = 0;
            for (const typed::TypedServerRequest& currentRequest : currentGenerationRequests) {
                acceptedCurrent += respondForLifecycle(currentRequest) ? 1U : 0U;
            }
            expect(acceptedCurrent == InitialRequestCount && state->outbound.size() == before + InitialRequestCount,
                   "all five current-generation occurrences answer through the same socket");
            const std::vector<codex::Json> expected{
                {{"id", "generation-patch"}, {"result", {{"decision", "denied"}}}},
                {{"id", "generation-exec"}, {"result", {{"decision", "timed_out"}}}},
                {{"id", "generation-command"}, {"result", {{"decision", "decline"}}}},
                {{"id", "generation-file"}, {"result", {{"decision", "cancel"}}}},
                {{"id", "generation-permissions"},
                 {"result",
                  {{"permissions",
                    {{"fileSystem",
                      {{"entries",
                        codex::Json::array({{{"access", "write"}, {"path", {{"type", "special"}, {"value", {{"kind", "slash_tmp"}}}}}}})},
                       {"globScanMaxDepth", 1},
                       {"read", nullptr},
                       {"write", codex::Json::array({"/synthetic/write"})}}},
                     {"network", {{"enabled", true}}}}},
                   {"scope", "session"},
                   {"strictAutoReview", false}}}},
            };
            std::size_t exact = 0;
            for (std::size_t index = 0; index < expected.size(); ++index) {
                const OutboundRecord& record = state->outbound[before + index];
                exact += record.envelope == expected[index] && record.line == expected[index].dump() + "\n" ? 1U : 0U;
            }
            expect(exact == InitialRequestCount, "all five reconnect responses retain exact IDs and response schemas");
            core::EventReceiver::atNextTick([this]() {
                client->stop();
            });
        }

        tests::support::TestResult& result;
        std::shared_ptr<UnixTransportState> state;
        bool socketReady = false;
        std::unique_ptr<TestClient> client;
        Phase phase = Phase::Initial;
        std::optional<typed::ApplyPatchApprovalRequest> patchRequest;
        std::optional<typed::ExecCommandApprovalRequest> execRequest;
        std::optional<typed::CommandApprovalRequest> commandRequest;
        std::optional<typed::FileChangeApprovalRequest> fileRequest;
        std::optional<typed::PermissionsApprovalRequest> permissionsRequest;
        std::vector<typed::TypedServerRequest> oldGenerationRequests;
        std::vector<typed::TypedServerRequest> currentGenerationRequests;
        std::set<std::string> rawMethods;
        std::size_t typedRequestCount = 0;
        std::size_t rawRequestCount = 0;
        std::size_t serverRequestBaseline = 0;
        std::size_t unexpectedProfileCallbacks = 0;
        int readyCount = 0;
        int stoppedCount = 0;
        bool insideTypedCallback = false;
        bool responseInsideCallback = false;
        bool callbackThrowObserved = false;
        bool finished = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    core::SNodeC::init(argc, argv);

    ApprovalWireRunner runner(result);
    runner.start();

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({15, 0}));
    const int startResult = core::SNodeC::start(utils::Timeval({16, 0}));

    result.expectTrue(!timedOut && runner.isFinished(), "A1.3 approval AF_UNIX lifecycle matrix completes before watchdog");
    result.expectEqual(0, startResult, "A1.3 approval lifecycle matrix stops the event loop cleanly");
    core::SNodeC::free();
    return result.processResult();
}
