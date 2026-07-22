/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/frontend/EventJournal.h"
#include "ai/openai/codex/frontend/UpdateBatch.h"
#include "support/TestResult.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    using namespace ai::openai::codex::frontend;

    void expectClientRoundTrip(tests::support::TestResult& result, const ClientMessage& message, const std::string& description) {
        const auto encoded = Codec::encodeClient(message);
        result.expectTrue(encoded.hasValue(), description + " encodes");
        if (!encoded) {
            return;
        }
        const auto decoded = Codec::decodeClient(encoded.value());
        result.expectTrue(decoded.hasValue(), description + " decodes");
        if (!decoded) {
            return;
        }
        const auto reencoded = Codec::encodeClient(decoded.value());
        result.expectTrue(reencoded.hasValue() && reencoded.value() == encoded.value(), description + " round-trips exactly");
    }

    void expectServerRoundTrip(tests::support::TestResult& result, const ServerMessage& message, const std::string& description) {
        const auto encoded = Codec::encodeServer(message);
        result.expectTrue(encoded.hasValue(), description + " encodes");
        if (!encoded) {
            return;
        }
        const auto decoded = Codec::decodeServer(encoded.value());
        result.expectTrue(decoded.hasValue(), description + " decodes");
        if (!decoded) {
            return;
        }
        const auto reencoded = Codec::encodeServer(decoded.value());
        result.expectTrue(reencoded.hasValue() && reencoded.value() == encoded.value(), description + " round-trips exactly");
    }

    Command command(std::string requestId, CommandParameters parameters) {
        return {std::move(requestId), std::move(parameters), Json::object(), Json::object()};
    }

    // Test-only evaluator for the draft-2020-12 keywords reached by FrontendEvent -> ItemState.
    // It keeps the checked-in schema, rather than a duplicated C++ predicate, authoritative for these examples.
    bool matchesJsonType(const Json& value, std::string_view type) {
        if (type == "object") {
            return value.is_object();
        }
        if (type == "array") {
            return value.is_array();
        }
        if (type == "string") {
            return value.is_string();
        }
        if (type == "boolean") {
            return value.is_boolean();
        }
        if (type == "integer") {
            return value.is_number_integer() || value.is_number_unsigned();
        }
        if (type == "number") {
            return value.is_number();
        }
        if (type == "null") {
            return value.is_null();
        }
        return false;
    }

    bool matchesSchema(const Json& root, const Json& schema, const Json& value, std::size_t depth = 0) {
        if (depth > 64) {
            return false;
        }
        if (schema.is_boolean()) {
            return schema.get<bool>();
        }
        if (!schema.is_object()) {
            return true;
        }

        if (const auto reference = schema.find("$ref"); reference != schema.end()) {
            constexpr std::string_view definitionsPrefix = "#/$defs/";
            if (!reference->is_string()) {
                return false;
            }
            const std::string name = reference->get<std::string>();
            if (!name.starts_with(definitionsPrefix)) {
                return false;
            }
            const auto definitions = root.find("$defs");
            const auto definition =
                definitions != root.end() ? definitions->find(name.substr(definitionsPrefix.size())) : Json::const_iterator{};
            if (definitions == root.end() || definition == definitions->end() || !matchesSchema(root, *definition, value, depth + 1)) {
                return false;
            }
        }

        if (const auto type = schema.find("type"); type != schema.end()) {
            bool typeMatched = false;
            if (type->is_string()) {
                typeMatched = matchesJsonType(value, type->get<std::string>());
            } else if (type->is_array()) {
                for (const Json& candidate : *type) {
                    typeMatched = typeMatched || (candidate.is_string() && matchesJsonType(value, candidate.get<std::string>()));
                }
            }
            if (!typeMatched) {
                return false;
            }
        }
        if (const auto constant = schema.find("const"); constant != schema.end() && value != *constant) {
            return false;
        }
        if (const auto enumeration = schema.find("enum");
            enumeration != schema.end() && enumeration->is_array() &&
            std::find(enumeration->begin(), enumeration->end(), value) == enumeration->end()) {
            return false;
        }
        if (value.is_number()) {
            if (const auto minimum = schema.find("minimum"); minimum != schema.end() && value < *minimum) {
                return false;
            }
            if (const auto maximum = schema.find("maximum"); maximum != schema.end() && value > *maximum) {
                return false;
            }
        }

        if (value.is_object()) {
            if (const auto required = schema.find("required"); required != schema.end() && required->is_array()) {
                for (const Json& name : *required) {
                    if (!name.is_string() || !value.contains(name.get<std::string>())) {
                        return false;
                    }
                }
            }
            if (const auto properties = schema.find("properties"); properties != schema.end() && properties->is_object()) {
                for (const auto& [name, propertySchema] : properties->items()) {
                    if (const auto member = value.find(name);
                        member != value.end() && !matchesSchema(root, propertySchema, *member, depth + 1)) {
                        return false;
                    }
                }
            }
        }
        if (value.is_array()) {
            if (const auto items = schema.find("items"); items != schema.end()) {
                for (const Json& item : value) {
                    if (!matchesSchema(root, *items, item, depth + 1)) {
                        return false;
                    }
                }
            }
        }

        if (const auto allOf = schema.find("allOf"); allOf != schema.end() && allOf->is_array()) {
            for (const Json& memberSchema : *allOf) {
                if (!matchesSchema(root, memberSchema, value, depth + 1)) {
                    return false;
                }
            }
        }
        if (const auto condition = schema.find("if"); condition != schema.end()) {
            const bool conditionMatched = matchesSchema(root, *condition, value, depth + 1);
            const auto branch = schema.find(conditionMatched ? "then" : "else");
            if (branch != schema.end() && !matchesSchema(root, *branch, value, depth + 1)) {
                return false;
            }
        }
        return true;
    }

    void testUserMessageDataSchema(tests::support::TestResult& result) {
        const std::filesystem::path sourceRoot = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
        const std::filesystem::path schemaPath = sourceRoot / "docs/ai/openai/codex/frontend-protocol-v1.schema.json";
        std::ifstream input(schemaPath);
        result.expectTrue(input.good(), "the checked-in Frontend Protocol v1 schema is readable");
        if (!input) {
            return;
        }

        const Json schema = Json::parse(input, nullptr, false);
        result.expectTrue(!schema.is_discarded() && schema.contains("$defs") && schema["$defs"].contains("UserMessageData"),
                          "the schema defines normalized user-message data explicitly");
        if (schema.is_discarded() || !schema.contains("$defs") || !schema["$defs"].contains("FrontendEvent")) {
            return;
        }

        const Json content =
            Json::array({Json{{"type", "text"}, {"text", "hello"}}, Json{{"type", "future"}, {"payload", Json{{"nested", true}}}}});
        const auto contentBytes = static_cast<std::uint64_t>(content.dump().size());
        const Json data = Json{{"clientId", nullptr},
                               {"content", content},
                               {"contentTruncated", false},
                               {"originalContentBytes", contentBytes},
                               {"retainedContentBytes", contentBytes},
                               {"originalContentItems", std::uint64_t{2}},
                               {"retainedContentItems", std::uint64_t{2}}};
        const Json item = Json{{"id", "item-1"},
                               {"type", "user_message"},
                               {"status", "completed"},
                               {"agentText", ""},
                               {"reasoningText", ""},
                               {"reasoningSummary", ""},
                               {"commandOutput", ""},
                               {"droppedContentBytes", std::uint64_t{0}},
                               {"contentTruncated", false},
                               {"data", data},
                               {"extensions", Json::object()}};
        const Json event = Json{{"sequence", std::uint64_t{1}},
                                {"type", "item.updated"},
                                {"data", Json{{"threadId", "thread-1"}, {"turnId", "turn-1"}, {"item", item}}}};
        const Json& frontendEventSchema = schema["$defs"]["FrontendEvent"];
        result.expectTrue(matchesSchema(schema, frontendEventSchema, event),
                          "the schema accepts array-valued normalized user-message content");

        Json invalidEvent = event;
        invalidEvent["data"]["item"]["data"]["content"] = Json{{"truncated", true}};
        result.expectTrue(!matchesSchema(schema, frontendEventSchema, invalidEvent),
                          "the schema rejects object-valued normalized user-message content");

        Json incompleteEvent = event;
        incompleteEvent["data"]["item"]["data"].erase("retainedContentItems");
        result.expectTrue(!matchesSchema(schema, frontendEventSchema, incompleteEvent),
                          "the schema requires user-message truncation metadata");

        Json futureItemEvent = invalidEvent;
        futureItemEvent["data"]["item"]["type"] = "future_item";
        result.expectTrue(matchesSchema(schema, frontendEventSchema, futureItemEvent),
                          "the user-message condition leaves future item data shapes unrestricted");
    }

    void testDefaultReplayHeadroom(tests::support::TestResult& result) {
        EventJournal journal;
        constexpr std::size_t retainedSlackBytes = 32;
        constexpr std::size_t maximumPayloadBytes = 24U * 1024U;
        const std::size_t retainedTarget = DefaultJournalMaxBytes - retainedSlackBytes;

        while (journal.retainedBytes() < retainedTarget) {
            const SequenceNumber next{journal.currentSequence().value() + 1};
            const std::size_t remaining = retainedTarget - journal.retainedBytes();
            const FrontendEvent empty{next,
                                      "item.content.updated",
                                      Json{{"itemId", "capacity-item"}, {"channel", "commandOutput"}, {"content", ""}},
                                      Json::object()};
            const auto emptySize = Codec::serializedEventSize(empty);
            result.expectTrue(emptySize.hasValue(), "capacity-test normalized event has a stable compact base encoding");
            if (!emptySize || remaining <= emptySize.value()) {
                break;
            }

            const std::size_t payloadBytes = std::min(maximumPayloadBytes, remaining - emptySize.value());
            const JournalAppendResult appended = journal.append(
                "item.content.updated",
                Json{{"itemId", "capacity-item"}, {"channel", "commandOutput"}, {"content", std::string(payloadBytes, 'x')}});
            result.expectTrue(appended.retained() && appended.serializedBytes <= remaining,
                              "capacity-test normalized event is retained without crossing the journal byte bound");
            if (!appended.retained() || appended.serializedBytes > remaining) {
                break;
            }
        }

        result.expectTrue(journal.retainedBytes() <= DefaultJournalMaxBytes && DefaultJournalMaxBytes - journal.retainedBytes() < 512 &&
                              journal.retainedEntryCount() <= DefaultJournalMaxEntries,
                          "default journal can be filled deterministically to within one small event of its 8 MiB bound");

        const JournalReplayResult replay = journal.replayAfter(SequenceNumber{0});
        const UpdateBatchResult batches = UpdateBatchBuilder{}.build(replay.events);
        result.expectTrue(replay.status == JournalReplayStatus::Available && batches.success(),
                          "the near-capacity journal remains a complete normalized replay");

        std::size_t queuedBytes = 0;
        const auto welcome = Codec::serializeServer(
            ServerMessage{Welcome{"capacity-session", SessionRole::Observer, journal.currentSequence(), SyncMode::Replay, Json::object()}});
        const auto complete = Codec::serializeServer(ServerMessage{SyncComplete{journal.currentSequence(), Json::object()}});
        result.expectTrue(welcome.hasValue() && complete.hasValue(), "capacity-test replay control envelopes serialize");
        if (welcome && complete) {
            queuedBytes = welcome.value().size() + complete.value().size();
        }
        bool boundedBatches = true;
        for (const BoundedEventBatch& batch : batches.batches) {
            boundedBatches = boundedBatches && batch.serializedBytes <= DefaultBatchMaxBytes;
            queuedBytes += batch.serializedBytes;
        }
        const std::size_t queuedMessages = batches.batches.size() + 2;
        result.expectTrue(boundedBatches && queuedBytes > DefaultJournalMaxBytes && queuedBytes <= DefaultAdapterMaxOutboundBytes &&
                              queuedMessages <= DefaultAdapterMaxOutboundMessages,
                          "default adapter headroom accepts exact batch/control overhead for a near-8 MiB retained replay");
    }
} // namespace

int main() {
    using namespace ai::openai::codex::frontend;

    tests::support::TestResult result;

    expectClientRoundTrip(result, ClientMessage{Hello{}}, "hello without replay position");
    expectClientRoundTrip(result, ClientMessage{Hello{SequenceNumber(123), Json{{"future", true}}}}, "hello with replay position");

    std::vector<Command> commands;
    commands.push_back(command("controller-acquire", ControllerAcquire{}));
    commands.push_back(command("controller-release", ControllerRelease{}));
    commands.push_back(command("snapshot", SnapshotGet{}));
    commands.push_back(command("replay", ReplayAfter{SequenceNumber(12)}));
    commands.push_back(command("thread-start", ThreadStart{"/tmp/project", "gpt-5", "openai", "on-request", "workspace-write", false}));
    commands.push_back(command("thread-resume", ThreadResume{"thread-1", "/tmp/project", "gpt-5", "openai", "never", "read-only"}));
    commands.push_back(command("thread-list", ThreadList{"cursor", 25, false, "needle"}));
    commands.push_back(command("thread-read", ThreadRead{"thread-1", true}));

    TurnStart turnStart;
    turnStart.threadId = "thread-1";
    turnStart.input = {TextInput{"hello", Json::object()},
                       ImageUrlInput{"https://example.invalid/image.png", "low", Json::object()},
                       LocalImageInput{"/tmp/image.png", "high", Json::object()},
                       SkillInput{"skill", "/tmp/SKILL.md", Json::object()},
                       MentionInput{"file", "/tmp/file.cpp", Json::object()}};
    turnStart.cwd = "/tmp/project";
    turnStart.model = "gpt-5";
    turnStart.reasoningEffort = "high";
    turnStart.approvalPolicy = "on-request";
    turnStart.sandboxPolicy = SandboxPolicy{"workspaceWrite", false, {"/tmp/project"}, false, true, Json::object()};
    commands.push_back(command("turn-start", std::move(turnStart)));
    commands.push_back(command("turn-interrupt", TurnInterrupt{"thread-1", "turn-1"}));
    commands.push_back(command("approval", ApprovalRespond{"41", "decline"}));
    commands.push_back(command("user-input", UserInputRespond{"42", {{"question-1", {"answer"}}}}));
    commands.push_back(command("authentication", AuthenticationRespond{"43", "secret-token", "account", "plus"}));
    commands.push_back(command("unknown-response", UnknownRequestRespond{"44", Json{{"accepted", false}}}));
    commands.push_back(command("unknown-reject", UnknownRequestReject{"45", -32001, "unsupported", Json{{"reason", "test"}}}));

    for (const Command& value : commands) {
        expectClientRoundTrip(result, ClientMessage{value}, "command " + value.requestId);
    }

    expectServerRoundTrip(
        result, ServerMessage{Welcome{"7", SessionRole::Observer, SequenceNumber(140), SyncMode::Replay, Json::object()}}, "welcome");
    expectServerRoundTrip(result, ServerMessage{SyncComplete{SequenceNumber(140), Json::object()}}, "sync complete");
    expectServerRoundTrip(result, ServerMessage{Snapshot{SequenceNumber(140), Json{{"lifecycle", "ready"}}, Json::object()}}, "snapshot");

    const FrontendEvent extensionEvent{
        SequenceNumber(141), "codex.extension", Json{{"method", "future/event"}, {"params", Json{{"value", 1}}}}, Json::object()};
    const FrontendEvent itemEvent{
        SequenceNumber(142), "item.content.updated", Json{{"itemId", "item-1"}, {"text", "complete"}}, Json::object()};
    expectServerRoundTrip(result,
                          ServerMessage{EventBatch{SequenceNumber(141), SequenceNumber(142), {extensionEvent, itemEvent}, Json::object()}},
                          "event batch including unknown Codex extension");
    expectServerRoundTrip(result, ServerMessage{Response::success("ok", Json{{"threadId", "thread-1"}})}, "success response");
    expectServerRoundTrip(result,
                          ServerMessage{Response::failure(
                              "failed", CommandError{ErrorCode::PermissionDenied, "controller required", std::nullopt, Json::object()})},
                          "error response");
    expectServerRoundTrip(
        result,
        ServerMessage{ProtocolErrorMessage{
            ErrorCode::UnsupportedVersion, "unsupported protocol version", {1}, true, std::nullopt, std::nullopt, Json::object()}},
        "protocol error");

    const std::vector<ErrorCode> stableErrors = {ErrorCode::PermissionDenied,
                                                 ErrorCode::InvalidCommand,
                                                 ErrorCode::NotFound,
                                                 ErrorCode::Conflict,
                                                 ErrorCode::LocalSubmissionFailure,
                                                 ErrorCode::TypedDecodingFailure,
                                                 ErrorCode::RemoteAppServerError,
                                                 ErrorCode::Cancelled,
                                                 ErrorCode::BackendUnavailable,
                                                 ErrorCode::DuplicateRequestId,
                                                 ErrorCode::MalformedJson,
                                                 ErrorCode::WrongProtocol,
                                                 ErrorCode::UnsupportedVersion,
                                                 ErrorCode::MissingField,
                                                 ErrorCode::InvalidField,
                                                 ErrorCode::UnknownKind,
                                                 ErrorCode::UnknownMethod,
                                                 ErrorCode::FrameTooLarge,
                                                 ErrorCode::CapacityExceeded,
                                                 ErrorCode::SequenceOverflow,
                                                 ErrorCode::ReplayGap,
                                                 ErrorCode::InternalError};
    for (const ErrorCode code : stableErrors) {
        result.expectTrue(errorCodeFromString(toString(code)) == code, "stable error code round-trips through its wire spelling");
    }

    const auto malformed = Codec::decodeClient(std::string_view("{not-json"));
    result.expectTrue(!malformed && malformed.error().code == ErrorCode::MalformedJson, "malformed JSON is rejected locally");
    const auto missingProtocol = Codec::decodeClient(Json{{"version", 1}, {"kind", "hello"}});
    result.expectTrue(!missingProtocol, "a missing protocol identity is rejected");
    const auto wrongProtocol = Codec::decodeClient(Json{{"protocol", "other"}, {"version", 1}, {"kind", "hello"}});
    result.expectTrue(!wrongProtocol && wrongProtocol.error().code == ErrorCode::WrongProtocol && wrongProtocol.error().closeConnection,
                      "a wrong protocol identity is rejected and closes only the frontend");
    const auto wrongVersion = Codec::decodeClient(Json{{"protocol", ProtocolIdentity}, {"version", 2}, {"kind", "hello"}});
    result.expectTrue(!wrongVersion && wrongVersion.error().code == ErrorCode::UnsupportedVersion &&
                          wrongVersion.error().supportedVersions == std::vector<std::uint32_t>{1},
                      "an unsupported version reports the supported version");
    const auto missingKind = Codec::decodeClient(Json{{"protocol", ProtocolIdentity}, {"version", 1}});
    result.expectTrue(!missingKind, "a missing message kind is rejected");
    const auto unknownKind = Codec::decodeClient(Json{{"protocol", ProtocolIdentity}, {"version", 1}, {"kind", "future-kind"}});
    result.expectTrue(!unknownKind && unknownKind.error().code == ErrorCode::UnknownKind, "an unknown message kind is stable error");
    const auto missingRequestId = Codec::decodeClient(
        Json{{"protocol", ProtocolIdentity}, {"version", 1}, {"kind", "command"}, {"method", "snapshot.get"}, {"params", Json::object()}});
    result.expectTrue(!missingRequestId, "a missing command request ID is rejected");
    const auto emptyRequestId = Codec::decodeClient(Json{{"protocol", ProtocolIdentity},
                                                         {"version", 1},
                                                         {"kind", "command"},
                                                         {"requestId", ""},
                                                         {"method", "snapshot.get"},
                                                         {"params", Json::object()}});
    result.expectTrue(!emptyRequestId, "an empty command request ID is rejected");
    const auto unknownMethod = Codec::decodeClient(Json{{"protocol", ProtocolIdentity},
                                                        {"version", 1},
                                                        {"kind", "command"},
                                                        {"requestId", "unknown"},
                                                        {"method", "future.command"},
                                                        {"params", Json::object()}});
    result.expectTrue(!unknownMethod && unknownMethod.error().code == ErrorCode::UnknownMethod, "an unknown command method is rejected");
    const auto malformedParams = Codec::decodeClient(Json{{"protocol", ProtocolIdentity},
                                                          {"version", 1},
                                                          {"kind", "command"},
                                                          {"requestId", "bad-params"},
                                                          {"method", "turn.interrupt"},
                                                          {"params", Json{{"threadId", 1}}}});
    result.expectTrue(!malformedParams, "malformed command-specific params are rejected");
    const auto invalidRange = Codec::decodeClient(Json{{"protocol", ProtocolIdentity},
                                                       {"version", 1},
                                                       {"kind", "command"},
                                                       {"requestId", "bad-range"},
                                                       {"method", "thread.list"},
                                                       {"params", Json{{"limit", std::uint64_t{1} << 40}}}});
    result.expectTrue(!invalidRange, "out-of-range command integers are rejected safely");
    const auto unknownField = Codec::decodeClient(
        Json{{"protocol", ProtocolIdentity}, {"version", 1}, {"kind", "hello"}, {"futureField", Json{{"nested", true}}}});
    result.expectTrue(unknownField.hasValue(), "unknown non-conflicting fields are tolerated");

    EventJournal journal({3, 4096, SequenceNumber(0)});
    const auto first = journal.append("thread.updated", Json{{"id", "thread-1"}});
    const auto second = journal.append("turn.updated", Json{{"id", "turn-1"}});
    const auto third = journal.append("item.updated", Json{{"id", "item-1"}});
    result.expectTrue(first.retained() && first.event->sequence == SequenceNumber(1), "journal starts at sequence one");
    result.expectTrue(second.event->sequence == SequenceNumber(2) && third.event->sequence == SequenceNumber(3),
                      "journal allocates strictly monotonic sequences");
    const auto afterOne = journal.replayAfter(SequenceNumber(1));
    result.expectTrue(afterOne.status == JournalReplayStatus::Available && afterOne.events.size() == 2 &&
                          afterOne.events.front().sequence == SequenceNumber(2),
                      "journal replays strictly after the requested sequence");
    result.expectTrue(journal.replayAfter(journal.currentSequence()).events.empty(), "replay from current sequence is empty");
    const std::vector<FrontendEvent> retainedCopy = journal.retainedEvents();
    const auto fourth = journal.append("controller.changed", Json{{"sessionId", "7"}});
    result.expectTrue(fourth.accepted(), "a fourth journal event is accepted before count eviction");
    result.expectTrue(journal.retainedEntryCount() == 3 && journal.replayAfter(SequenceNumber(0)).requiresSnapshot(),
                      "entry-count eviction creates an explicit replay gap");
    result.expectTrue(retainedCopy.front().type == "thread.updated", "copies of retained events are not mutated by later eviction");

    EventJournal byteJournal({16, 180, SequenceNumber(0)});
    for (int index = 0; index < 6; ++index) {
        const auto appended = byteJournal.append("item.content.updated", Json{{"text", std::string(40, static_cast<char>('a' + index))}});
        result.expectTrue(appended.accepted(), "byte-bound journal accepts a bounded event");
    }
    result.expectTrue(byteJournal.retainedBytes() <= 180 && byteJournal.retainedEntryCount() < 6,
                      "journal byte eviction enforces the configured total-byte bound");

    EventJournal overflowJournal({1, 1024, SequenceNumber(std::numeric_limits<std::uint64_t>::max())});
    result.expectTrue(overflowJournal.append("event", Json::object()).status == JournalAppendStatus::SequenceOverflow,
                      "sequence overflow fails explicitly without wrapping");

    EventJournal barrierJournal({4, 4096, SequenceNumber(0)});
    const auto beforeBarrier = barrierJournal.append("thread.updated", Json{{"id", "before-barrier"}});
    const SequenceNumber beforeBarrierSequence = barrierJournal.currentSequence();
    result.expectTrue(beforeBarrier.accepted() && barrierJournal.invalidateReplay() &&
                          barrierJournal.currentSequence() > beforeBarrierSequence &&
                          barrierJournal.replayAfter(beforeBarrierSequence).requiresSnapshot() &&
                          barrierJournal.replayAfter(barrierJournal.currentSequence()).status == JournalReplayStatus::Available,
                      "replay invalidation advances a monotonic snapshot barrier and rejects stale current-sequence replay");
    EventJournal exhaustedBarrier({1, 1024, SequenceNumber(std::numeric_limits<std::uint64_t>::max())});
    result.expectTrue(!exhaustedBarrier.invalidateReplay() &&
                          exhaustedBarrier.currentSequence() == SequenceNumber(std::numeric_limits<std::uint64_t>::max()) &&
                          exhaustedBarrier.replayAfter(exhaustedBarrier.currentSequence()).requiresSnapshot(),
                      "snapshot barrier exhaustion fails explicitly without wrapping and keeps replay unavailable");

    std::vector<FrontendEvent> batchEvents;
    for (std::uint64_t sequence = 1; sequence <= 7; ++sequence) {
        batchEvents.push_back({SequenceNumber(sequence), "item.updated", Json{{"sequence", sequence}}, Json::object()});
    }
    UpdateBatchBuilder builder({3, 1024});
    const auto batches = builder.build(batchEvents);
    result.expectTrue(batches.success() && batches.batches.size() == 3, "event-count bounds split updates into bounded batches");
    std::uint64_t expectedSequence = 1;
    bool ordered = true;
    for (const BoundedEventBatch& batch : batches.batches) {
        ordered = ordered && batch.batch.events.size() <= 3 && batch.serializedBytes <= 1024;
        for (const FrontendEvent& event : batch.batch.events) {
            ordered = ordered && event.sequence.value() == expectedSequence++;
        }
    }
    result.expectTrue(ordered, "bounded batching preserves stable sequence order and serialized-size bounds");

    UpdateBatchBuilder tinyBuilder({8, 120});
    const auto oversized =
        tinyBuilder.build({{SequenceNumber(1), "item.content.updated", Json{{"text", std::string(1024, 'x')}}, Json::object()}});
    result.expectTrue(oversized.requiresSnapshot() && oversized.oversizedSequence == SequenceNumber(1),
                      "a single oversized update requests snapshot fallback");

    testDefaultReplayHeadroom(result);
    testUserMessageDataSchema(result);

    return result.processResult();
}
