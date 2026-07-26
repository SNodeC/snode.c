/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/Transport.h"
#include "ai/openai/codex/typed/Client.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Types.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <nlohmann/detail/iterators/iter_impl.hpp>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    using Submission = codex::AppServerClient::RawProtocol::Submission;
    constexpr std::size_t OperationCount = 10;

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
            {"codexHome", "/tmp/codex-a1-3-filesystem-wire"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-a1-3-filesystem-wire/1"},
        };
    }

    codex::Json metadataResult() {
        return {
            {"createdAtMs", std::numeric_limits<std::int64_t>::min()},
            {"futureMetadataField", true},
            {"isDirectory", false},
            {"isFile", true},
            {"isSymlink", false},
            {"modifiedAtMs", std::numeric_limits<std::int64_t>::max()},
        };
    }

    codex::Json directoryResult() {
        return {
            {"entries",
             codex::Json::array({
                 {
                     {"fileName", "first.bin"},
                     {"isDirectory", false},
                     {"isFile", true},
                 },
                 {
                     {"fileName", "second directory"},
                     {"isDirectory", true},
                     {"isFile", false},
                 },
             })},
            {"futureDirectoryField", true},
        };
    }

    codex::Json readFileResult() {
        return {
            {"dataBase64", "bm90LWEtbG9jYWwtZmlsZQ=="},
            {"futureReadField", true},
        };
    }

    codex::Json watchResult() {
        return {
            {"path", "/synthetic/server/watch path/back\\slash"},
            {"futureWatchField", true},
        };
    }

    codex::Json
    fuzzyFile(std::string fileName, codex::Json indices, std::string matchType, std::string path, std::string root, std::uint32_t score) {
        return {
            {"file_name", std::move(fileName)},
            {"indices", std::move(indices)},
            {"match_type", std::move(matchType)},
            {"path", std::move(path)},
            {"root", std::move(root)},
            {"score", score},
        };
    }

    codex::Json fuzzyResult() {
        return {
            {"files",
             codex::Json::array({
                 fuzzyFile("first.cpp", codex::Json::array({0, 2}), "file", "src/first.cpp", "/synthetic/root-b", 90),
                 fuzzyFile("second directory", nullptr, "directory", "src/second directory", "/synthetic/root-a", 70),
             })},
            {"futureFuzzyField", true},
        };
    }

    struct OutboundRecord {
        std::string line;
        codex::Json envelope;
    };

    struct UnixTransportState {
        enum class ReplyMode { Success, RemoteError, Hold };

        detail::TransportCallbacks callbacks;
        std::vector<detail::TransportCallbacks> callbackGenerations;
        std::vector<OutboundRecord> outbound;
        std::map<std::string, codex::Json> successResults;
        ReplyMode replyMode = ReplyMode::Success;
        int clientDescriptor = -1;
        int serverDescriptor = -1;
        bool emitWatchChanged = false;
        bool emitFuzzyNotifications = false;
        bool duplicateReadFileResult = false;
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
            const std::string serverLine = envelope.dump() + "\n";
            if (!writeFully(serverDescriptor, serverLine)) {
                return false;
            }
            const std::optional<std::string> received = readLine(clientDescriptor);
            if (!received || *received != serverLine || !target.onMessage) {
                return false;
            }
            target.onMessage(received->substr(0, received->size() - 1));
            return true;
        }

        bool injectCurrent(const codex::Json& envelope) {
            return injectWith(callbacks, envelope);
        }

        bool injectWatchChanged() {
            return injectCurrent({
                {"jsonrpc", "2.0"},
                {"method", "fs/changed"},
                {"params",
                 {
                     {"changedPaths", codex::Json::array({"/synthetic/watch/first", "/synthetic/watch/path with spaces/back\\slash"})},
                     {"futureParam", true},
                     {"watchId", "watch/wire-id"},
                 }},
                {"futureEnvelopeField", true},
            });
        }

        bool injectFuzzyNotifications() {
            const codex::Json future =
                fuzzyFile("future.file", codex::Json::array({1}), "future-match", "future/path", "/synthetic/root-a", 50);
            return injectCurrent({
                       {"jsonrpc", "2.0"},
                       {"method", "fuzzyFileSearch/sessionUpdated"},
                       {"params",
                        {
                            {"files", codex::Json::array({future})},
                            {"query", "wire query"},
                            {"sessionId", "stable/session-wire"},
                        }},
                   }) &&
                   injectCurrent({
                       {"jsonrpc", "2.0"},
                       {"method", "fuzzyFileSearch/sessionCompleted"},
                       {"params", {{"sessionId", "stable/session-wire"}}},
                   });
        }

        bool send(std::string message) {
            const std::string clientLine = message + "\n";
            if (!writeFully(clientDescriptor, clientLine)) {
                return false;
            }
            const std::optional<std::string> received = readLine(serverDescriptor);
            if (!received || *received != clientLine) {
                return false;
            }

            codex::Json envelope = codex::Json::parse(received->begin(), received->end() - 1, nullptr, false);
            if (envelope.is_discarded()) {
                return false;
            }
            outbound.push_back({*received, envelope});

            const auto method = envelope.find("method");
            if (method == envelope.end() || !method->is_string()) {
                return true;
            }
            if (*method == "initialize") {
                const auto id = envelope.find("id");
                return id != envelope.end() && injectCurrent({{"id", *id}, {"result", initializeResult()}});
            }
            if (*method == "initialized" || envelope.find("id") == envelope.end()) {
                return true;
            }
            if (replyMode == ReplyMode::Hold) {
                return true;
            }

            const std::string operation = *method;
            if (replyMode == ReplyMode::RemoteError) {
                return injectCurrent({{"id", envelope.at("id")},
                                      {"error",
                                       {{"code", -32'414},
                                        {"message", "synthetic filesystem remote failure"},
                                        {"data", {{"operation", operation}}},
                                        {"futureErrorField", true}}}});
            }

            if (operation == "fs/watch" && emitWatchChanged && !injectWatchChanged()) {
                return false;
            }
            if (operation == "fuzzyFileSearch" && emitFuzzyNotifications && !injectFuzzyNotifications()) {
                return false;
            }

            const auto result = successResults.find(operation);
            if (result == successResults.end() || !injectCurrent({{"id", envelope.at("id")}, {"result", result->second}})) {
                return false;
            }
            if (operation == "fs/readFile" && duplicateReadFileResult) {
                return injectCurrent({{"id", envelope.at("id")}, {"result", result->second}});
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
                              {"codex_a1_3_filesystem_wire_test", "Codex A1.3 Filesystem Wire Test", "1"}) {
        }
    };

    enum class Phase { LocalRejection, Success, RemoteError, Cancellation, Generation };

    struct OperationCase {
        std::string method;
        detail::ClientRequestTarget target;
        detail::ResultContractKind resultKind;
        codex::Json expectedParams;
        codex::Json expectedResult;
        std::function<Submission(Phase)> invoke;
    };

    class FilesystemWireRunner {
    public:
        explicit FilesystemWireRunner(tests::support::TestResult& result)
            : result(result)
            , state(std::make_shared<UnixTransportState>())
            , socketReady(state->open())
            , client(std::make_unique<TestClient>(state)) {
            buildCases();
        }

        void start() {
            expect(socketReady, "A1.3 filesystem wire test opens its AF_UNIX socketpair");
            if (!socketReady) {
                finished = true;
                core::SNodeC::stop();
                return;
            }

            client->typed().events().setOnEvent([this](const typed::Event& event) {
                if (const auto* changed = std::get_if<typed::FsChangedNotification>(&event)) {
                    ++typedChangedEvents;
                    expect(changed->watchId.value == "watch/wire-id" && changed->changedPaths.size() == 2 &&
                               changed->changedPaths[1].value == "/synthetic/watch/path with spaces/back\\slash" &&
                               changed->raw.contains("futureEnvelopeField"),
                           "typed fs/changed retains watch correlation, path bytes, and raw");
                    if (!reentrantSubmitted) {
                        reentrantSubmitted = true;
                        const Submission submission = client->typed().filesystem().readFile(
                            {{"/synthetic/no-local-access/reentrant.bin"}},
                            [this](const typed::OperationResult<typed::FsReadFileResponse>& operation) {
                                expect(operation.kind == typed::OperationResult<typed::FsReadFileResponse>::Kind::Success &&
                                           operation.value && operation.value->dataBase64 == "bm90LWEtbG9jYWwtZmlsZQ==" &&
                                           operation.raw == readFileResult(),
                                       "reentrant readFile completes through the same RawProtocol");
                                ++reentrantCallbacks;
                                maybeCompleteSuccess();
                            });
                        expect(static_cast<bool>(submission) && submission.id && !submission.error,
                               "an fs/changed callback can submit a reentrant filesystem request");
                    }
                    eventCallbackThrew = true;
                    throw std::runtime_error("intentional fs/changed callback failure");
                }
                if (const auto* updated = std::get_if<typed::FuzzyFileSearchSessionUpdatedNotification>(&event)) {
                    ++typedFuzzyUpdatedEvents;
                    expect(updated->sessionId == "stable/session-wire" && updated->query == "wire query" && updated->files.size() == 1 &&
                               updated->files[0].matchType.value == "future-match" && !updated->files[0].matchType.isKnown() &&
                               updated->diagnostics.size() == 1,
                           "stable fuzzy update preserves session, ordering, and future match");
                }
                if (const auto* completed = std::get_if<typed::FuzzyFileSearchSessionCompletedNotification>(&event)) {
                    ++typedFuzzyCompletedEvents;
                    expect(completed->sessionId == "stable/session-wire", "stable fuzzy completion retains its session id");
                }
                if (const auto* unknown = std::get_if<typed::UnknownEvent>(&event); unknown && unknown->method == "fs/changed") {
                    ++malformedTypedEvents;
                    expect(unknown->diagnostic && unknown->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                               unknown->diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning &&
                               unknown->raw.at("params").at("changedPaths").at(0) == 7,
                           "malformed fs/changed remains typed-observable and diagnosed");
                    maybeBeginRemoteErrors();
                }
                maybeCompleteSuccess();
            });
            client->raw().setOnNotification([this](const codex::Notification& notification) {
                if (notification.method == "fs/changed") {
                    if (notification.params.is_object() &&
                        notification.params.value("watchId", codex::Json{}) == codex::Json("watch/wire-id")) {
                        ++rawChangedEvents;
                    } else {
                        ++malformedRawEvents;
                        maybeBeginRemoteErrors();
                    }
                } else if (notification.method == "fuzzyFileSearch/sessionUpdated") {
                    ++rawFuzzyUpdatedEvents;
                } else if (notification.method == "fuzzyFileSearch/sessionCompleted") {
                    ++rawFuzzyCompletedEvents;
                }
                maybeCompleteSuccess();
            });

            client->setOnStateChanged([this](const codex::StateChange& change) {
                if (change.current == codex::State::Ready) {
                    ++readyCount;
                    if (readyCount == 1) {
                        beginSuccess();
                    } else if (readyCount == 2) {
                        beginGeneration();
                    }
                } else if (change.current == codex::State::Failed) {
                    expect(false, "A1.3 filesystem socket transcript must not fail");
                    client->stop();
                } else if (change.current == codex::State::Stopped) {
                    if (readyCount == 1) {
                        firstStopObserved = true;
                        maybeRestart();
                    } else if (readyCount == 2) {
                        finished = true;
                        core::SNodeC::stop();
                    }
                }
            });

            invokeLocalRejections();
            client->start();
        }

        bool isFinished() const noexcept {
            return finished;
        }

    private:
        void expect(bool condition, const std::string& description) {
            result.expectTrue(condition, description);
        }

        std::vector<std::string> expectedMethods() const {
            std::vector<std::string> methods;
            methods.reserve(cases.size());
            for (const OperationCase& operation : cases) {
                methods.push_back(operation.method);
            }
            return methods;
        }

        template <typename Result>
        bool typedRawMatches(const typed::OperationResult<Result>& operation, const codex::Json& expected) {
            if (!operation.value) {
                return false;
            }
            if constexpr (std::is_same_v<Result, typed::Unit>) {
                return expected == codex::Json::object();
            } else {
                return operation.value->raw == expected;
            }
        }

        template <typename Result>
        void expectConcreteFields(const typed::OperationResult<Result>& operation, const std::string& method) {
            if (!operation.value) {
                return;
            }
            if constexpr (std::is_same_v<Result, typed::FsGetMetadataResponse>) {
                expect(operation.value->createdAtMs == std::numeric_limits<std::int64_t>::min() &&
                           operation.value->modifiedAtMs == std::numeric_limits<std::int64_t>::max() && operation.value->isFile,
                       "fs/getMetadata exposes exact metadata and integer bounds");
            } else if constexpr (std::is_same_v<Result, typed::FsReadDirectoryResponse>) {
                expect(operation.value->entries.size() == 2 && operation.value->entries[0].fileName == "first.bin" &&
                           operation.value->entries[1].fileName == "second directory",
                       "fs/readDirectory preserves server entry ordering");
            } else if constexpr (std::is_same_v<Result, typed::FsReadFileResponse>) {
                expect(operation.value->dataBase64 == "bm90LWEtbG9jYWwtZmlsZQ==",
                       "fs/readFile preserves the server's base64-bearing value");
            } else if constexpr (std::is_same_v<Result, typed::FsWatchResponse>) {
                expect(operation.value->path.value == "/synthetic/server/watch path/back\\slash",
                       "fs/watch preserves the returned server path bytes");
            } else if constexpr (std::is_same_v<Result, typed::FuzzyFileSearchResponse>) {
                expect(operation.value->files.size() == 2 && operation.value->files[0].fileName == "first.cpp" &&
                           operation.value->files[1].fileName == "second directory" && operation.value->files[1].indices.isNull(),
                       "fuzzyFileSearch preserves result order and nullable indices");
            } else {
                static_cast<void>(method);
            }
        }

        template <typename Result>
        std::function<void(const typed::OperationResult<Result>&)> handler(std::string method, codex::Json expectedResult, Phase phase) {
            return [this, method = std::move(method), expectedResult = std::move(expectedResult), phase](
                       const typed::OperationResult<Result>& operation) {
                expect(!insideSubmission, method + " completion remains asynchronous");
                if (phase == Phase::LocalRejection) {
                    ++unexpectedLocalCallbacks;
                    expect(false, method + " synchronous rejection invokes no callback");
                    return;
                }
                if (phase == Phase::Success || phase == Phase::Generation) {
                    expect(operation.kind == typed::OperationResult<Result>::Kind::Success && operation.value &&
                               operation.raw == expectedResult && typedRawMatches(operation, expectedResult),
                           method + " decodes its authoritative result and retains exact raw");
                    expectConcreteFields(operation, method);
                    auto& order = phase == Phase::Success ? successOrder : generationOrder;
                    auto& count = phase == Phase::Success ? successCallbacks : generationCallbacks;
                    order.push_back(method);
                    ++count;
                    if (phase == Phase::Success) {
                        maybeCompleteSuccess();
                        if (method == "fs/remove") {
                            completionCallbackThrew = true;
                            throw std::runtime_error("intentional filesystem completion callback failure");
                        }
                    } else if (count == OperationCount) {
                        core::EventReceiver::atNextTick([this]() {
                            completeGeneration();
                        });
                    }
                    return;
                }
                if (phase == Phase::RemoteError) {
                    const codex::Json expectedError{
                        {"code", -32'414},
                        {"message", "synthetic filesystem remote failure"},
                        {"data", {{"operation", method}}},
                        {"futureErrorField", true},
                    };
                    expect(operation.kind == typed::OperationResult<Result>::Kind::RemoteError && !operation.value &&
                               operation.remoteError && operation.remoteError->code == -32'414 &&
                               operation.remoteError->message == "synthetic filesystem remote failure" &&
                               operation.remoteError->raw == expectedError && operation.raw.is_null(),
                           method + " retains its exact JSON-RPC error");
                    remoteOrder.push_back(method);
                    if (++remoteCallbacks == OperationCount) {
                        core::EventReceiver::atNextTick([this]() {
                            beginCancellation();
                        });
                    }
                    return;
                }

                expect(operation.kind == typed::OperationResult<Result>::Kind::Cancelled && !operation.value && operation.localError &&
                           operation.localError->category == codex::Error::Category::Cancelled,
                       method + " receives one cancellation at the connection boundary");
                cancellationOrder.push_back(method);
                ++cancellationCallbacks;
                maybeRestart();
            };
        }

        template <typename Result, typename Params, typename Submit>
        OperationCase makeOperation(std::string method,
                                    detail::ClientRequestTarget target,
                                    detail::ResultContractKind resultKind,
                                    Params params,
                                    codex::Json expectedParams,
                                    codex::Json expectedResult,
                                    Submit submit) {
            state->successResults.emplace(method, expectedResult);
            return {
                method,
                target,
                resultKind,
                std::move(expectedParams),
                expectedResult,
                [this, method, expectedResult, params = std::move(params), submit = std::move(submit)](Phase phase) mutable {
                    return submit(params, handler<Result>(method, expectedResult, phase));
                },
            };
        }

        void buildCases() {
            auto& filesystem = client->typed().filesystem();
            const std::string path = "/synthetic/no-local-access/path with spaces/back\\slash";
            cases.push_back(makeOperation<typed::Unit>("fs/copy",
                                                       detail::ClientRequestTarget::FsCopy,
                                                       detail::ResultContractKind::Unit,
                                                       typed::FsCopyParams{
                                                           .destinationPath = {"/synthetic/no-local-access/destination"},
                                                           .recursive = false,
                                                           .sourcePath = {path},
                                                       },
                                                       {
                                                           {"destinationPath", "/synthetic/no-local-access/destination"},
                                                           {"recursive", false},
                                                           {"sourcePath", path},
                                                       },
                                                       codex::Json::object(),
                                                       [&filesystem](auto params, auto resultHandler) {
                                                           return filesystem.copy(std::move(params), std::move(resultHandler));
                                                       }));
            cases.push_back(makeOperation<typed::Unit>("fs/createDirectory",
                                                       detail::ClientRequestTarget::FsCreateDirectory,
                                                       detail::ResultContractKind::Unit,
                                                       typed::FsCreateDirectoryParams{
                                                           .path = {path},
                                                           .recursive = typed::OptionalNullable<bool>::explicitNull(),
                                                       },
                                                       {
                                                           {"path", path},
                                                           {"recursive", nullptr},
                                                       },
                                                       codex::Json::object(),
                                                       [&filesystem](auto params, auto resultHandler) {
                                                           return filesystem.createDirectory(std::move(params), std::move(resultHandler));
                                                       }));
            cases.push_back(makeOperation<typed::FsGetMetadataResponse>("fs/getMetadata",
                                                                        detail::ClientRequestTarget::FsGetMetadata,
                                                                        detail::ResultContractKind::Concrete,
                                                                        typed::FsGetMetadataParams{{path}},
                                                                        {{"path", path}},
                                                                        metadataResult(),
                                                                        [&filesystem](auto params, auto resultHandler) {
                                                                            return filesystem.getMetadata(std::move(params),
                                                                                                          std::move(resultHandler));
                                                                        }));
            cases.push_back(makeOperation<typed::FsReadDirectoryResponse>("fs/readDirectory",
                                                                          detail::ClientRequestTarget::FsReadDirectory,
                                                                          detail::ResultContractKind::Concrete,
                                                                          typed::FsReadDirectoryParams{{path}},
                                                                          {{"path", path}},
                                                                          directoryResult(),
                                                                          [&filesystem](auto params, auto resultHandler) {
                                                                              return filesystem.readDirectory(std::move(params),
                                                                                                              std::move(resultHandler));
                                                                          }));
            cases.push_back(makeOperation<typed::FsReadFileResponse>("fs/readFile",
                                                                     detail::ClientRequestTarget::FsReadFile,
                                                                     detail::ResultContractKind::Concrete,
                                                                     typed::FsReadFileParams{{path}},
                                                                     {{"path", path}},
                                                                     readFileResult(),
                                                                     [&filesystem](auto params, auto resultHandler) {
                                                                         return filesystem.readFile(std::move(params),
                                                                                                    std::move(resultHandler));
                                                                     }));
            cases.push_back(makeOperation<typed::Unit>("fs/remove",
                                                       detail::ClientRequestTarget::FsRemove,
                                                       detail::ResultContractKind::Unit,
                                                       typed::FsRemoveParams{
                                                           .force = typed::OptionalNullable<bool>::explicitNull(),
                                                           .path = {path},
                                                           .recursive = typed::OptionalNullable<bool>::withValue(true),
                                                       },
                                                       {
                                                           {"force", nullptr},
                                                           {"path", path},
                                                           {"recursive", true},
                                                       },
                                                       codex::Json::object(),
                                                       [&filesystem](auto params, auto resultHandler) {
                                                           return filesystem.remove(std::move(params), std::move(resultHandler));
                                                       }));
            cases.push_back(makeOperation<typed::FsWatchResponse>("fs/watch",
                                                                  detail::ClientRequestTarget::FsWatch,
                                                                  detail::ResultContractKind::Concrete,
                                                                  typed::FsWatchParams{{path}, {"watch/wire-id"}},
                                                                  {
                                                                      {"path", path},
                                                                      {"watchId", "watch/wire-id"},
                                                                  },
                                                                  watchResult(),
                                                                  [&filesystem](auto params, auto resultHandler) {
                                                                      return filesystem.watch(std::move(params), std::move(resultHandler));
                                                                  }));
            cases.push_back(makeOperation<typed::Unit>("fs/unwatch",
                                                       detail::ClientRequestTarget::FsUnwatch,
                                                       detail::ResultContractKind::Unit,
                                                       typed::FsUnwatchParams{{"watch/wire-id"}},
                                                       {{"watchId", "watch/wire-id"}},
                                                       codex::Json::object(),
                                                       [&filesystem](auto params, auto resultHandler) {
                                                           return filesystem.unwatch(std::move(params), std::move(resultHandler));
                                                       }));
            cases.push_back(makeOperation<typed::Unit>("fs/writeFile",
                                                       detail::ClientRequestTarget::FsWriteFile,
                                                       detail::ResultContractKind::Unit,
                                                       typed::FsWriteFileParams{"not-canonical_base64==", {path}},
                                                       {
                                                           {"dataBase64", "not-canonical_base64=="},
                                                           {"path", path},
                                                       },
                                                       codex::Json::object(),
                                                       [&filesystem](auto params, auto resultHandler) {
                                                           return filesystem.writeFile(std::move(params), std::move(resultHandler));
                                                       }));
            cases.push_back(makeOperation<typed::FuzzyFileSearchResponse>(
                "fuzzyFileSearch",
                detail::ClientRequestTarget::FuzzyFileSearch,
                detail::ResultContractKind::Concrete,
                typed::FuzzyFileSearchParams{
                    .cancellationToken = typed::OptionalNullable<std::string>::withValue("cancellation/wire"),
                    .query = "wire query",
                    .roots = {"/synthetic/root-b", "/synthetic/root-a"},
                },
                {
                    {"cancellationToken", "cancellation/wire"},
                    {"query", "wire query"},
                    {"roots", codex::Json::array({"/synthetic/root-b", "/synthetic/root-a"})},
                },
                fuzzyResult(),
                [&filesystem](auto params, auto resultHandler) {
                    return filesystem.fuzzyFileSearch(std::move(params), std::move(resultHandler));
                }));
        }

        void invokeLocalRejections() {
            std::size_t exact = 0;
            for (OperationCase& operation : cases) {
                insideSubmission = true;
                const Submission submission = operation.invoke(Phase::LocalRejection);
                insideSubmission = false;
                const detail::ProtocolSurfaceEntry& entry = detail::entryFor(operation.target);
                const detail::ProtocolSurfaceKey expectedKey{
                    detail::SurfaceCategory::ClientRequest,
                    "ClientRequest",
                    "method",
                    operation.method,
                };
                const auto* registeredTarget = std::get_if<detail::ClientRequestTarget>(&entry.runtimeTarget);
                const bool registryExact = entry.key == expectedKey && registeredTarget != nullptr &&
                                           *registeredTarget == operation.target &&
                                           entry.operationContract.resultKind == operation.resultKind;
                const bool rejected =
                    !submission && !submission.id && submission.error && submission.error->category == codex::Error::Category::InvalidState;
                exact += rejected && registryExact ? 1U : 0U;
                expect(rejected, operation.method + " rejects synchronously before RawProtocol is ready");
                expect(registryExact, operation.method + " starts from its exact registry target and result kind");
            }
            expect(exact == OperationCount && unexpectedLocalCallbacks == 0,
                   "all ten filesystem/fuzzy operations reject locally without callbacks");
        }

        void invokeBatch(Phase phase) {
            const std::size_t before = state->outbound.size();
            std::vector<std::optional<codex::ClientRequestId>> ids;
            ids.reserve(cases.size());
            for (OperationCase& operation : cases) {
                insideSubmission = true;
                const Submission submission = operation.invoke(phase);
                insideSubmission = false;
                expect(static_cast<bool>(submission) && submission.id && !submission.error,
                       operation.method + " is accepted by the one RawProtocol engine");
                ids.push_back(submission.id);
            }
            expect(state->outbound.size() == before + cases.size(), "all ten operations cross the AF_UNIX transport exactly once");
            for (std::size_t index = 0; index < cases.size(); ++index) {
                const codex::Json expectedEnvelope{
                    {"id", ids[index] ? codex::Json(ids[index]->value()) : codex::Json(nullptr)},
                    {"method", cases[index].method},
                    {"params", cases[index].expectedParams},
                };
                const OutboundRecord& record = state->outbound[before + index];
                expect(record.envelope == expectedEnvelope && record.line == expectedEnvelope.dump() + "\n",
                       cases[index].method + " sends exact JSON-RPC/JSONL bytes");
            }
            if (phase == Phase::Generation) {
                generationIds = std::move(ids);
            }
        }

        void beginSuccess() {
            state->replyMode = UnixTransportState::ReplyMode::Success;
            state->emitWatchChanged = true;
            state->emitFuzzyNotifications = true;
            state->duplicateReadFileResult = true;
            invokeBatch(Phase::Success);
        }

        void maybeCompleteSuccess() {
            if (successCompletionScheduled || successCallbacks != OperationCount || reentrantCallbacks != 1 || typedChangedEvents != 1 ||
                typedFuzzyUpdatedEvents != 1 || typedFuzzyCompletedEvents != 1 || rawChangedEvents != 1 || rawFuzzyUpdatedEvents != 1 ||
                rawFuzzyCompletedEvents != 1) {
                return;
            }
            successCompletionScheduled = true;
            core::EventReceiver::atNextTick([this]() {
                completeSuccess();
            });
        }

        void completeSuccess() {
            expect(successOrder == expectedMethods() && successCallbacks == OperationCount,
                   "filesystem success callbacks complete exactly once in submission order");
            expect(eventCallbackThrew && completionCallbackThrew, "filesystem event and completion callback exceptions are contained");
            expect(reentrantSubmitted && reentrantCallbacks == 1, "filesystem event reentrancy produces exactly one completion");
            expect(typedChangedEvents == 1 && rawChangedEvents == 1 && typedFuzzyUpdatedEvents == 1 && rawFuzzyUpdatedEvents == 1 &&
                       typedFuzzyCompletedEvents == 1 && rawFuzzyCompletedEvents == 1,
                   "watch and both stable fuzzy notifications use existing typed/raw dispatch");
            expect(successCallbacks == OperationCount, "a duplicate readFile response does not duplicate completion");

            const auto reentrant = std::find_if(state->outbound.begin(), state->outbound.end(), [](const OutboundRecord& record) {
                return record.envelope.value("method", "") == "fs/readFile" && record.envelope.contains("params") &&
                       record.envelope.at("params").value("path", "") == "/synthetic/no-local-access/reentrant.bin";
            });
            expect(reentrant != state->outbound.end() && reentrant->line == reentrant->envelope.dump() + "\n",
                   "reentrant readFile uses the same exact JSONL path");
            expect(std::all_of(cases.begin(),
                               cases.end(),
                               [](const OperationCase& operation) {
                                   return operation.expectedParams.dump().find("/synthetic/no-local-access") != std::string::npos ||
                                          operation.method == "fs/unwatch" || operation.method == "fuzzyFileSearch";
                               }),
                   "synthetic non-local paths are transmitted as protocol data only");

            state->emitWatchChanged = false;
            state->emitFuzzyNotifications = false;
            state->duplicateReadFileResult = false;
            expect(state->injectCurrent({
                       {"jsonrpc", "2.0"},
                       {"method", "fs/changed"},
                       {"params",
                        {
                            {"changedPaths", codex::Json::array({7})},
                            {"watchId", "watch/malformed"},
                        }},
                   }),
                   "malformed known fs/changed crosses the socket");
        }

        void maybeBeginRemoteErrors() {
            if (remoteScheduled || malformedTypedEvents != 1 || malformedRawEvents != 1) {
                return;
            }
            remoteScheduled = true;
            core::EventReceiver::atNextTick([this]() {
                beginRemoteErrors();
            });
        }

        void beginRemoteErrors() {
            state->replyMode = UnixTransportState::ReplyMode::RemoteError;
            invokeBatch(Phase::RemoteError);
        }

        void beginCancellation() {
            expect(remoteCallbacks == OperationCount && remoteOrder == expectedMethods(),
                   "all filesystem JSON-RPC failures retain callback order");
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            invokeBatch(Phase::Cancellation);
            client->stop();
        }

        void maybeRestart() {
            if (!restartScheduled && firstStopObserved && cancellationCallbacks == OperationCount) {
                restartScheduled = true;
                expect(cancellationOrder == expectedMethods(), "held filesystem operations cancel exactly once in submission order");
                core::EventReceiver::atNextTick([this]() {
                    client->start();
                });
            }
        }

        void beginGeneration() {
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            invokeBatch(Phase::Generation);
            expect(state->callbackGenerations.size() >= 2, "filesystem restart installs a distinct transport generation");
            if (state->callbackGenerations.size() < 2) {
                client->stop();
                return;
            }

            const detail::TransportCallbacks stale = state->callbackGenerations.front();
            for (std::size_t index = 0; index < cases.size(); ++index) {
                if (generationIds[index]) {
                    expect(state->injectWith(stale, {{"id", generationIds[index]->value()}, {"result", cases[index].expectedResult}}),
                           cases[index].method + " stale response bytes cross the socket");
                }
            }
            core::EventReceiver::atNextTick([this]() {
                expect(generationCallbacks == 0, "stale filesystem responses are rejected before delivery");
                const detail::TransportCallbacks current = state->callbackGenerations.back();
                for (std::size_t index = 0; index < cases.size(); ++index) {
                    if (generationIds[index]) {
                        expect(state->injectWith(current, {{"id", generationIds[index]->value()}, {"result", cases[index].expectedResult}}),
                               cases[index].method + " current response bytes cross the socket");
                    }
                }
            });
        }

        void completeGeneration() {
            expect(generationCallbacks == OperationCount && generationOrder == expectedMethods(),
                   "current-generation filesystem responses complete once without stale "
                   "resurrection");
            client->stop();
        }

        tests::support::TestResult& result;
        std::shared_ptr<UnixTransportState> state;
        bool socketReady = false;
        std::unique_ptr<TestClient> client;
        std::vector<OperationCase> cases;
        std::vector<std::string> successOrder;
        std::vector<std::string> remoteOrder;
        std::vector<std::string> cancellationOrder;
        std::vector<std::string> generationOrder;
        std::vector<std::optional<codex::ClientRequestId>> generationIds;
        std::size_t successCallbacks = 0;
        std::size_t remoteCallbacks = 0;
        std::size_t cancellationCallbacks = 0;
        std::size_t generationCallbacks = 0;
        std::size_t reentrantCallbacks = 0;
        std::size_t typedChangedEvents = 0;
        std::size_t rawChangedEvents = 0;
        std::size_t typedFuzzyUpdatedEvents = 0;
        std::size_t rawFuzzyUpdatedEvents = 0;
        std::size_t typedFuzzyCompletedEvents = 0;
        std::size_t rawFuzzyCompletedEvents = 0;
        std::size_t malformedTypedEvents = 0;
        std::size_t malformedRawEvents = 0;
        std::size_t unexpectedLocalCallbacks = 0;
        int readyCount = 0;
        bool insideSubmission = false;
        bool reentrantSubmitted = false;
        bool eventCallbackThrew = false;
        bool completionCallbackThrew = false;
        bool successCompletionScheduled = false;
        bool remoteScheduled = false;
        bool firstStopObserved = false;
        bool restartScheduled = false;
        bool finished = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    core::SNodeC::init(argc, argv);

    FilesystemWireRunner runner(result);
    runner.start();

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({20, 0}));
    const int startResult = core::SNodeC::start(utils::Timeval({21, 0}));

    result.expectTrue(!timedOut && runner.isFinished(), "A1.3 filesystem AF_UNIX lifecycle matrix completes before watchdog");
    result.expectEqual(0, startResult, "A1.3 filesystem lifecycle matrix stops the event loop cleanly");
    core::SNodeC::free();
    return result.processResult();
}
