/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/detail/ProtocolCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/ServerRequestDecoder.h"
#include "ai/openai/codex/detail/ThreadCodec.h"
#include "support/TestResult.h"

#include <cerrno>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

#ifndef CODEX_PHASE_A0_TRANSCRIPT_FIXTURES
#error "CODEX_PHASE_A0_TRANSCRIPT_FIXTURES must name the checked-in transcript directory"
#endif

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;
    using ai::openai::codex::Json;
    using ai::openai::codex::Notification;
    using ai::openai::codex::ServerRequest;
    using ai::openai::codex::ServerRequestId;
    using ai::openai::codex::ServerRequestToken;

    class Descriptor {
    public:
        Descriptor() = default;

        explicit Descriptor(int descriptor)
            : value(descriptor) {
        }

        ~Descriptor() {
            if (value >= 0) {
                close(value);
            }
        }

        Descriptor(const Descriptor&) = delete;
        Descriptor& operator=(const Descriptor&) = delete;

        int get() const noexcept {
            return value;
        }

    private:
        int value = -1;
    };

    std::string fixture(std::string_view name) {
        std::ifstream input(std::string(CODEX_PHASE_A0_TRANSCRIPT_FIXTURES) + "/" + std::string(name), std::ios::binary);
        if (!input) {
            return {};
        }
        return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
    }

    bool writeFully(int descriptor, std::string_view bytes) {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const ssize_t written = write(descriptor, bytes.data() + offset, bytes.size() - offset);
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

    std::optional<std::string> readExactly(int descriptor, std::size_t byteCount) {
        std::string bytes(byteCount, '\0');
        std::size_t offset = 0;
        while (offset < byteCount) {
            const ssize_t received = read(descriptor, bytes.data() + offset, byteCount - offset);
            if (received > 0) {
                offset += static_cast<std::size_t>(received);
            } else if (received < 0 && errno == EINTR) {
                continue;
            } else {
                return std::nullopt;
            }
        }
        return bytes;
    }

    std::optional<std::string> transfer(int writer, int reader, std::string_view expected) {
        if (!writeFully(writer, expected)) {
            return std::nullopt;
        }
        return readExactly(reader, expected.size());
    }

    std::optional<detail::ProtocolMessage> decodeJsonLine(const std::string& line, std::string& error) {
        if (line.empty() || line.back() != '\n' || line.find('\n') != line.size() - 1 || line.find('\r') != std::string::npos) {
            error = "fixture is not one exactly LF-framed JSONL record";
            return std::nullopt;
        }
        return detail::ProtocolCodec::decode(std::string_view(line).substr(0, line.size() - 1), error);
    }

    Notification asNotification(const detail::ProtocolMessage& message) {
        return {message.method, message.params, message.raw};
    }

    ServerRequest asServerRequest(const detail::ProtocolMessage& message) {
        return {ServerRequestId{*message.id}, message.method, message.params, message.raw, ServerRequestToken{1}};
    }
} // namespace

int main() {
    tests::support::TestResult result;
    int descriptors[2]{-1, -1};
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors) != 0) {
        result.expectTrue(false, "AF_UNIX socketpair creation succeeds");
        return result.processResult();
    }
    Descriptor client(descriptors[0]);
    Descriptor server(descriptors[1]);

    const std::string requestFixture = fixture("thread-list-request.jsonl");
    std::string encodingError;
    std::string parameterError;
    const std::optional<Json> params = detail::encodeThreadListParams(typed::ThreadListOptions{}, parameterError);
    const detail::ProtocolSurfaceEntry& requestEntry = detail::entryFor(detail::ClientRequestTarget::ThreadList);
    const std::optional<std::string> encodedRequest =
        params ? detail::ProtocolCodec::encodeRequest(7, requestEntry.key.name, *params, encodingError) : std::nullopt;
    const std::string outbound = encodedRequest ? *encodedRequest + "\n" : std::string();
    const std::optional<std::string> receivedRequest = transfer(client.get(), server.get(), outbound);
    result.expectTrue(parameterError.empty() && encodingError.empty() && outbound == requestFixture && receivedRequest == requestFixture,
                      "existing typed thread/list request crosses a real Unix stream as exact schema-validated JSONL bytes");

    std::string decodingError;
    const std::optional<detail::ProtocolMessage> decodedRequest =
        receivedRequest ? decodeJsonLine(*receivedRequest, decodingError) : std::nullopt;
    result.expectTrue(decodedRequest && decodedRequest->kind == detail::ProtocolMessage::Kind::Request &&
                          decodedRequest->method == requestEntry.key.name && decodedRequest->params == Json::object(),
                      "received request retains its registered exact wire method and empty typed parameters");

    const std::string deltaFixture = fixture("agent-message-delta.jsonl");
    const std::optional<std::string> receivedDelta = transfer(server.get(), client.get(), deltaFixture);
    const std::optional<detail::ProtocolMessage> deltaMessage =
        receivedDelta ? decodeJsonLine(*receivedDelta, decodingError) : std::nullopt;
    const typed::Event deltaEvent = deltaMessage ? detail::decodeEvent(asNotification(*deltaMessage))
                                                 : typed::Event{typed::UnknownEvent{"missing", nullptr, nullptr, decodingError}};
    const typed::AgentMessageDelta* delta = std::get_if<typed::AgentMessageDelta>(&deltaEvent);
    result.expectTrue(receivedDelta == deltaFixture && deltaMessage && deltaMessage->kind == detail::ProtocolMessage::Kind::Notification &&
                          delta && delta->text == "hello" && delta->threadId.value == "thread-1" && delta->turnId.value == "turn-1" &&
                          delta->itemId.value == "item-1",
                      "registry-based typed notification dispatch consumes exact bytes read from the Unix stream");

    const std::string itemFixture = fixture("item-started.jsonl");
    const std::optional<std::string> receivedItem = transfer(server.get(), client.get(), itemFixture);
    const std::optional<detail::ProtocolMessage> itemMessage = receivedItem ? decodeJsonLine(*receivedItem, decodingError) : std::nullopt;
    const typed::Event itemEvent = itemMessage ? detail::decodeEvent(asNotification(*itemMessage))
                                               : typed::Event{typed::UnknownEvent{"missing", nullptr, nullptr, decodingError}};
    const typed::ItemStarted* started = std::get_if<typed::ItemStarted>(&itemEvent);
    const typed::AgentMessageItem* agent = started ? std::get_if<typed::AgentMessageItem>(&started->item) : nullptr;
    const detail::ProtocolSurfaceEntry* itemEntry =
        detail::findSurface(detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "agentMessage");
    result.expectTrue(receivedItem == itemFixture && started && started->startedAtMs == 101 && agent &&
                          agent->metadata.id.value == "item-2" && agent->text == "complete" && itemEntry &&
                          std::get_if<detail::ItemDiscriminatorTarget>(&itemEntry->runtimeTarget) != nullptr,
                      "registry-based item/event discriminator dispatch consumes exact Unix JSONL bytes");

    const std::string serverRequestFixture = fixture("command-approval-request.jsonl");
    const std::optional<std::string> receivedServerRequest = transfer(server.get(), client.get(), serverRequestFixture);
    const std::optional<detail::ProtocolMessage> serverRequestMessage =
        receivedServerRequest ? decodeJsonLine(*receivedServerRequest, decodingError) : std::nullopt;
    const typed::TypedServerRequest typedServerRequest =
        serverRequestMessage ? detail::decodeServerRequest(asServerRequest(*serverRequestMessage))
                             : typed::TypedServerRequest{typed::UnknownServerRequest{
                                   ServerRequestId{0}, ServerRequestToken{}, "missing", nullptr, nullptr, decodingError}};
    const typed::CommandApprovalRequest* approval = std::get_if<typed::CommandApprovalRequest>(&typedServerRequest);
    result.expectTrue(receivedServerRequest == serverRequestFixture && serverRequestMessage &&
                          serverRequestMessage->kind == detail::ProtocolMessage::Kind::Request && approval &&
                          approval->threadId.value == "thread-1" && approval->turnId.value == "turn-1" &&
                          approval->itemId.value == "item-3" && approval->command == "printf OK" && approval->cwd == "/tmp/project",
                      "registry-based typed server-request dispatch consumes exact Unix JSONL bytes");

    backend::ReducerOptions reducerOptions;
    reducerOptions.retainedExtensions = 1;
    backend::Reducer reducer(reducerOptions);
    backend::BackendState state;
    bool unknownWireAndDecode = true;
    for (std::string_view name : {"unknown-notification-a.jsonl", "unknown-notification-b.jsonl"}) {
        const std::string unknownFixture = fixture(name);
        const std::optional<std::string> receivedUnknown = transfer(server.get(), client.get(), unknownFixture);
        const std::optional<detail::ProtocolMessage> unknownMessage =
            receivedUnknown ? decodeJsonLine(*receivedUnknown, decodingError) : std::nullopt;
        const typed::Event unknownEvent = unknownMessage ? detail::decodeEvent(asNotification(*unknownMessage))
                                                         : typed::Event{typed::UnknownEvent{"missing", nullptr, nullptr, decodingError}};
        const typed::UnknownEvent* unknown = std::get_if<typed::UnknownEvent>(&unknownEvent);
        unknownWireAndDecode =
            unknownWireAndDecode && receivedUnknown == unknownFixture && unknownMessage && unknown && unknown->raw == unknownMessage->raw;
        for (const backend::BackendEvent& event : reducer.translate(unknownEvent)) {
            reducer.apply(state, event);
        }
    }
    result.expectTrue(unknownWireAndDecode && state.recentExtensions.size() == 1 &&
                          state.recentExtensions.front().method == "future/opaque-next" &&
                          state.recentExtensions.front().payload.at("nested").at("secret") == "still-preserved",
                      "unknown Unix input reaches the bounded raw extension-preservation path without data loss");

    return result.processResult();
}
