/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/Snapshot.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace ai::openai::codex::backend {

    namespace {
        constexpr std::size_t MaxExtensionBytes = 64U * 1024U;
        constexpr std::size_t MaxExtensionNestingDepth = 32;
        constexpr std::size_t MaxExtensionJsonNodes = 4096;

        template <typename... Visitors>
        struct Overloaded : Visitors... {
            using Visitors::operator()...;
        };

        template <typename... Visitors>
        Overloaded(Visitors...) -> Overloaded<Visitors...>;

        Json boundedJson(const Json& value) {
            try {
                const std::string encoded = value.dump();
                if (encoded.size() <= MaxExtensionBytes) {
                    return value;
                }
                return Json::object(
                    {{"truncated", true}, {"originalBytes", encoded.size()}, {"preview", encoded.substr(0, MaxExtensionBytes)}});
            } catch (...) {
                return Json::object({{"omitted", true}, {"reason", "value could not be serialized safely"}});
            }
        }

        std::string safeUtf8Prefix(std::string_view value, std::size_t byteLimit) {
            std::size_t offset = 0;
            while (offset < value.size() && offset < byteLimit) {
                const unsigned char first = static_cast<unsigned char>(value[offset]);
                std::size_t width = 0;
                if (first <= 0x7fU) {
                    width = 1;
                } else if (first >= 0xc2U && first <= 0xdfU) {
                    width = 2;
                } else if (first >= 0xe0U && first <= 0xefU) {
                    width = 3;
                } else if (first >= 0xf0U && first <= 0xf4U) {
                    width = 4;
                } else {
                    break;
                }
                if (offset + width > value.size() || offset + width > byteLimit) {
                    break;
                }
                bool valid = true;
                for (std::size_t index = 1; index < width; ++index) {
                    const unsigned char continuation = static_cast<unsigned char>(value[offset + index]);
                    valid = valid && (continuation & 0xc0U) == 0x80U;
                }
                if (!valid) {
                    break;
                }
                offset += width;
            }
            return std::string(value.substr(0, offset));
        }

        std::string normalizedKey(std::string_view key) {
            std::string result;
            result.reserve(key.size());
            for (const char rawCharacter : key) {
                const unsigned char character = static_cast<unsigned char>(rawCharacter);
                if (std::isalnum(character) != 0) {
                    result.push_back(static_cast<char>(std::tolower(character)));
                }
            }
            return result;
        }

        bool isSensitiveKey(std::string_view key) {
            const std::string normalized = normalizedKey(key);
            return normalized.ends_with("token") || normalized.ends_with("secret") || normalized.ends_with("answer") ||
                   normalized.ends_with("answers") || normalized.ends_with("credential") || normalized.ends_with("credentials") ||
                   normalized.find("password") != std::string::npos || normalized.find("passphrase") != std::string::npos ||
                   normalized.find("authorization") != std::string::npos || normalized.find("apikey") != std::string::npos;
        }

        bool isSecretValueKey(std::string_view key) {
            const std::string normalized = normalizedKey(key);
            return normalized == "value" || normalized == "values" || normalized == "text" || normalized == "response" ||
                   normalized == "responses";
        }

        struct JsonSanitizerState {
            std::size_t remainingNodes = MaxExtensionJsonNodes;
            bool truncated = false;
            bool redacted = false;
        };

        Json sanitizeExtensionJson(const Json& value, JsonSanitizerState& state, std::size_t depth = 0) {
            if (depth >= MaxExtensionNestingDepth || state.remainingNodes == 0) {
                state.truncated = true;
                return Json::object({{"omitted", true}, {"reason", "extension structure limit exceeded"}});
            }
            --state.remainingNodes;

            if (value.is_object()) {
                Json result = Json::object();
                bool secretObject = false;
                if (const auto secret = value.find("secret"); secret != value.end() && secret->is_boolean()) {
                    secretObject = secret->get<bool>();
                }
                for (const auto& [key, member] : value.items()) {
                    if (isSensitiveKey(key) || (secretObject && isSecretValueKey(key))) {
                        result[key] = "[redacted]";
                        state.redacted = true;
                    } else if (normalizedKey(key) == "secret" && !member.is_boolean()) {
                        result[key] = "[redacted]";
                        state.redacted = true;
                    } else {
                        result[key] = sanitizeExtensionJson(member, state, depth + 1);
                    }
                }
                return result;
            }
            if (value.is_array()) {
                Json result = Json::array();
                for (const Json& member : value) {
                    if (state.remainingNodes == 0) {
                        state.truncated = true;
                        result.push_back(Json::object({{"omitted", true}, {"reason", "extension structure limit exceeded"}}));
                        break;
                    }
                    result.push_back(sanitizeExtensionJson(member, state, depth + 1));
                }
                return result;
            }
            return value;
        }

        std::string lifecycleName(ItemLifecycle lifecycle) {
            switch (lifecycle) {
                case ItemLifecycle::Unknown:
                    return "unknown";
                case ItemLifecycle::Started:
                    return "started";
                case ItemLifecycle::Completed:
                    return "completed";
                case ItemLifecycle::Failed:
                    return "failed";
            }
            return "unknown";
        }

        std::string errorCategoryName(Error::Category category) {
            switch (category) {
                case Error::Category::Launch:
                    return "launch";
                case Error::Category::Transport:
                    return "transport";
                case Error::Category::Protocol:
                    return "protocol";
                case Error::Category::Initialization:
                    return "initialization";
                case Error::Category::Process:
                    return "process";
                case Error::Category::InvalidState:
                    return "invalid_state";
                case Error::Category::Capacity:
                    return "capacity";
                case Error::Category::Cancelled:
                    return "cancelled";
                case Error::Category::Enqueue:
                    return "enqueue";
            }
            return "unknown";
        }

        ItemSnapshot snapshotItem(const typed::ItemId& id, const ItemState& state) {
            ItemSnapshot snapshot;
            snapshot.id = id.value;
            snapshot.type = itemType(state.item);
            snapshot.status = lifecycleName(state.lifecycle);
            snapshot.agentText = state.agentText;
            snapshot.reasoningText = state.reasoningText;
            snapshot.reasoningSummary = state.reasoningSummary;
            snapshot.commandOutput = state.commandOutput;
            snapshot.droppedContentBytes = state.droppedContentBytes;
            snapshot.contentTruncated = state.droppedContentBytes != 0;
            snapshot.startedAtMs = state.startedAtMs;
            snapshot.completedAtMs = state.completedAtMs;
            snapshot.extensions = boundedJson(state.extensions);

            std::visit(Overloaded{[&snapshot](const typed::AgentMessageItem& value) {
                                      if (value.phase) {
                                          snapshot.data["phase"] = *value.phase;
                                      }
                                  },
                                  [&snapshot](const typed::UserMessageItem& value) {
                                      snapshot.data = Json::object({{"clientId", value.clientId ? Json(*value.clientId) : Json(nullptr)},
                                                                    {"content", boundedJson(value.content)}});
                                  },
                                  [&snapshot](const typed::ReasoningItem&) {
                                      snapshot.data["hasSummary"] = !snapshot.reasoningSummary.empty();
                                  },
                                  [&snapshot](const typed::CommandExecutionItem& value) {
                                      snapshot.data =
                                          Json::object({{"command", value.command}, {"cwd", value.cwd}, {"status", value.status}});
                                      if (value.processId) {
                                          snapshot.data["processId"] = *value.processId;
                                      }
                                      if (value.exitCode) {
                                          snapshot.data["exitCode"] = *value.exitCode;
                                      }
                                      if (value.durationMs) {
                                          snapshot.data["durationMs"] = *value.durationMs;
                                      }
                                  },
                                  [&snapshot](const typed::FileChangeItem& value) {
                                      snapshot.data = Json::object({{"status", value.status}, {"changes", boundedJson(value.changes)}});
                                  },
                                  [&snapshot](const typed::ToolCallItem& value) {
                                      snapshot.data = Json::object(
                                          {{"tool", value.tool}, {"status", value.status}, {"hasResult", !value.result.is_null()}});
                                      if (value.server) {
                                          snapshot.data["server"] = *value.server;
                                      }
                                      if (value.nameSpace) {
                                          snapshot.data["namespace"] = *value.nameSpace;
                                      }
                                  },
                                  [&snapshot](const typed::WebSearchItem& value) {
                                      snapshot.data = Json::object({{"query", value.query}});
                                  },
                                  [&snapshot](const typed::UnknownItem& value) {
                                      snapshot.data = Json::object();
                                      if (value.type) {
                                          snapshot.data["codexType"] = *value.type;
                                      }
                                      if (value.decodingError) {
                                          snapshot.data["decodingError"] = *value.decodingError;
                                      }
                                  }},
                       state.item);
            return snapshot;
        }

        TurnSnapshot snapshotTurn(const typed::TurnId& id, const TurnState& state) {
            TurnSnapshot snapshot;
            snapshot.id = id.value;
            snapshot.threadId = state.turn.threadId.value;
            snapshot.status = state.turn.status.value;
            snapshot.active = state.active;
            snapshot.terminal = state.terminal;
            if (state.failure) {
                snapshot.failure = boundedJson(*state.failure);
            }
            if (state.tokenUsage) {
                snapshot.tokenUsage = boundedJson(*state.tokenUsage);
            }
            snapshot.extensions = boundedJson(state.extensions);

            std::set<std::string> visited;
            for (const typed::ItemId& itemIdValue : state.itemOrder) {
                const auto iterator = state.items.find(itemIdValue.value);
                if (iterator != state.items.end() && visited.insert(iterator->first).second) {
                    snapshot.items.push_back(snapshotItem(itemIdValue, iterator->second));
                }
            }
            for (const auto& [itemIdValue, item] : state.items) {
                if (visited.insert(itemIdValue).second) {
                    snapshot.items.push_back(snapshotItem(typed::ItemId{itemIdValue}, item));
                }
            }
            return snapshot;
        }

        ThreadSnapshot snapshotThread(const typed::ThreadId& id, const ThreadState& state) {
            ThreadSnapshot snapshot;
            snapshot.id = id.value;
            snapshot.title = state.thread.title;
            snapshot.cwd = state.thread.cwd;
            if (state.thread.model) {
                snapshot.model = state.thread.model->value;
            }
            snapshot.modelProvider = state.thread.modelProvider;
            snapshot.preview = state.thread.preview;
            if (state.thread.status) {
                snapshot.status = state.thread.status->value;
            }
            snapshot.createdAt = state.thread.createdAt;
            snapshot.updatedAt = state.thread.updatedAt;
            snapshot.fullyLoaded = state.fullyLoaded;
            snapshot.extensions = boundedJson(state.extensions);

            std::set<std::string> visited;
            for (const typed::TurnId& turnId : state.turnOrder) {
                const auto iterator = state.turns.find(turnId.value);
                if (iterator != state.turns.end() && visited.insert(iterator->first).second) {
                    snapshot.turns.push_back(snapshotTurn(turnId, iterator->second));
                }
            }
            for (const auto& [turnId, turn] : state.turns) {
                if (visited.insert(turnId).second) {
                    snapshot.turns.push_back(snapshotTurn(typed::TurnId{turnId}, turn));
                }
            }
            return snapshot;
        }

        PendingRequestSnapshot snapshotPendingRequest(const PendingRequestState& state) {
            PendingRequestSnapshot snapshot;
            snapshot.id = state.id;
            std::visit(Overloaded{[&snapshot](const typed::CommandApprovalRequest& value) {
                                      snapshot.type = "command_approval";
                                      snapshot.threadId = value.threadId.value;
                                      snapshot.turnId = value.turnId.value;
                                      snapshot.itemId = value.itemId.value;
                                      if (value.command) {
                                          snapshot.details["command"] = *value.command;
                                      }
                                      if (value.cwd) {
                                          snapshot.details["cwd"] = *value.cwd;
                                      }
                                      if (value.reason) {
                                          snapshot.details["reason"] = *value.reason;
                                      }
                                  },
                                  [&snapshot](const typed::FileChangeApprovalRequest& value) {
                                      snapshot.type = "file_change_approval";
                                      snapshot.threadId = value.threadId.value;
                                      snapshot.turnId = value.turnId.value;
                                      snapshot.itemId = value.itemId.value;
                                      if (value.reason) {
                                          snapshot.details["reason"] = *value.reason;
                                      }
                                      if (value.grantRoot) {
                                          snapshot.details["grantRoot"] = *value.grantRoot;
                                      }
                                  },
                                  [&snapshot](const typed::UserInputRequest& value) {
                                      snapshot.type = "user_input";
                                      snapshot.threadId = value.threadId.value;
                                      snapshot.turnId = value.turnId.value;
                                      snapshot.itemId = value.itemId.value;
                                      snapshot.details["questions"] = Json::array();
                                      for (const typed::UserInputQuestion& question : value.questions) {
                                          Json encoded = Json::object({{"id", question.id},
                                                                       {"header", question.header},
                                                                       {"prompt", question.prompt},
                                                                       {"allowsFreeText", question.allowsFreeText},
                                                                       {"secret", question.secret},
                                                                       {"options", Json::array()}});
                                          for (const typed::UserInputOption& option : question.options) {
                                              encoded["options"].push_back({{"label", option.label}, {"description", option.description}});
                                          }
                                          snapshot.details["questions"].push_back(std::move(encoded));
                                      }
                                      if (value.autoResolutionMs) {
                                          snapshot.details["autoResolutionMs"] = *value.autoResolutionMs;
                                      }
                                  },
                                  [&snapshot](const typed::AuthenticationRequest& value) {
                                      snapshot.type = "authentication";
                                      snapshot.details["reason"] = value.reason;
                                      if (value.previousAccountId) {
                                          snapshot.details["previousAccountId"] = *value.previousAccountId;
                                      }
                                  },
                                  [&snapshot](const typed::UnknownServerRequest& value) {
                                      snapshot.type = "unknown";
                                      const ExtensionSnapshot safeParams = makeExtensionSnapshot(ExtensionRecord{
                                          value.method, value.params, value.decodingError, std::nullopt, std::nullopt, std::nullopt});
                                      snapshot.details["method"] = safeParams.method;
                                      if (safeParams.methodTruncated) {
                                          snapshot.details["methodTruncated"] = true;
                                          snapshot.details["originalMethodBytes"] = safeParams.originalMethodBytes;
                                      }
                                      snapshot.details["params"] = safeParams.payload;
                                      if (safeParams.sensitiveFieldsRedacted) {
                                          snapshot.details["sensitiveFieldsRedacted"] = true;
                                      }
                                      if (safeParams.payloadTruncated) {
                                          snapshot.details["paramsTruncated"] = true;
                                          if (safeParams.originalPayloadBytes.has_value()) {
                                              snapshot.details["originalParamsBytes"] = *safeParams.originalPayloadBytes;
                                          }
                                      }
                                      if (safeParams.decodingError) {
                                          snapshot.details["decodingError"] = *safeParams.decodingError;
                                      }
                                      if (safeParams.decodingErrorTruncated) {
                                          snapshot.details["decodingErrorTruncated"] = true;
                                          snapshot.details["originalDecodingErrorBytes"] = safeParams.originalDecodingErrorBytes;
                                      }
                                  }},
                       state.request);
            snapshot.details = boundedJson(snapshot.details);
            return snapshot;
        }
    } // namespace

    ExtensionSnapshot makeExtensionSnapshot(const ExtensionRecord& extension) {
        ExtensionSnapshot snapshot;
        snapshot.originalMethodBytes = extension.originalMethodBytes.value_or(static_cast<std::uint64_t>(extension.method.size()));
        snapshot.method = safeUtf8Prefix(extension.method, MaxSnapshotExtensionMethodBytes);
        snapshot.methodTruncated = extension.originalMethodBytes.has_value() || snapshot.method.size() != extension.method.size();
        if (snapshot.method.empty()) {
            snapshot.method = "codex/unknown";
            snapshot.methodTruncated = true;
        }

        if (extension.decodingError) {
            snapshot.originalDecodingErrorBytes =
                extension.originalDecodingErrorBytes.value_or(static_cast<std::uint64_t>(extension.decodingError->size()));
            snapshot.decodingError = safeUtf8Prefix(*extension.decodingError, MaxSnapshotExtensionDecodingErrorBytes);
            snapshot.decodingErrorTruncated =
                extension.originalDecodingErrorBytes.has_value() || snapshot.decodingError->size() != extension.decodingError->size();
        }

        try {
            const std::string originalPayload = extension.payload.dump();
            snapshot.originalPayloadBytes = extension.originalPayloadBytes.value_or(static_cast<std::uint64_t>(originalPayload.size()));
            if (extension.payload.is_object() && extension.payload.value("truncated", false)) {
                snapshot.payloadTruncated = true;
                const auto originalBytes = extension.payload.find("originalBytes");
                if (originalBytes != extension.payload.end() && originalBytes->is_number_unsigned()) {
                    snapshot.originalPayloadBytes = originalBytes->get<std::uint64_t>();
                }
            }
            if (originalPayload.size() > MaxSnapshotExtensionPayloadBytes) {
                snapshot.payload = Json::object({{"omitted", true},
                                                 {"reason", "extension payload exceeds frontend snapshot bound"},
                                                 {"originalBytes", originalPayload.size()}});
                snapshot.payloadTruncated = true;
                return snapshot;
            }

            JsonSanitizerState sanitizer;
            snapshot.payload = sanitizeExtensionJson(extension.payload, sanitizer);
            snapshot.sensitiveFieldsRedacted = sanitizer.redacted;
            snapshot.payloadTruncated = snapshot.payloadTruncated || sanitizer.truncated;
            if (snapshot.payload.dump().size() > MaxSnapshotExtensionPayloadBytes) {
                snapshot.payload = Json::object({{"omitted", true},
                                                 {"reason", "sanitized extension payload exceeds frontend snapshot bound"},
                                                 {"originalBytes", originalPayload.size()}});
                snapshot.payloadTruncated = true;
            }
        } catch (...) {
            snapshot.payload = Json::object({{"omitted", true}, {"reason", "extension payload could not be serialized safely"}});
            snapshot.payloadTruncated = true;
        }
        return snapshot;
    }

    bool Snapshot::operator==(const Snapshot& other) const {
        return sequence == other.sequence && lifecycle == other.lifecycle && lastLifecycleError == other.lastLifecycleError &&
               diagnostics.received == other.diagnostics.received && diagnostics.recent == other.diagnostics.recent &&
               threads == other.threads && pendingRequests == other.pendingRequests && controller == other.controller &&
               sessions == other.sessions && threadList == other.threadList && replayRange == other.replayRange &&
               recentExtensions == other.recentExtensions && omittedRecentExtensions == other.omittedRecentExtensions &&
               sequenceExhausted == other.sequenceExhausted;
    }

    bool Snapshot::operator!=(const Snapshot& other) const {
        return !(*this == other);
    }

    Snapshot makeSnapshot(const BackendState& state) {
        Snapshot snapshot;
        snapshot.sequence = state.sequence;
        snapshot.lifecycle = state.lifecycle;
        if (state.lastLifecycleError) {
            snapshot.lastLifecycleError = ErrorSnapshot{
                errorCategoryName(state.lastLifecycleError->category), state.lastLifecycleError->code, state.lastLifecycleError->message};
        }
        snapshot.diagnostics = state.diagnostics;
        snapshot.controller = state.controller;
        snapshot.threadList = ThreadListSnapshot{state.threadList.hasLoadedPage,
                                                 state.threadList.complete,
                                                 state.threadList.nextCursor,
                                                 state.threadList.backwardsCursor,
                                                 state.threadList.pagesLoaded};
        snapshot.sequenceExhausted = state.sequenceExhausted;

        std::set<std::string> visited;
        for (const typed::ThreadId& threadId : state.threadOrder) {
            const auto iterator = state.threads.find(threadId.value);
            if (iterator != state.threads.end() && visited.insert(iterator->first).second) {
                snapshot.threads.push_back(snapshotThread(threadId, iterator->second));
            }
        }
        for (const auto& [threadId, thread] : state.threads) {
            if (visited.insert(threadId).second) {
                snapshot.threads.push_back(snapshotThread(typed::ThreadId{threadId}, thread));
            }
        }
        for (const auto& [id, pending] : state.pendingRequests) {
            (void) id;
            snapshot.pendingRequests.push_back(snapshotPendingRequest(pending));
        }
        for (const auto& [id, session] : state.sessions) {
            (void) id;
            snapshot.sessions.push_back({session.id, session.role});
        }
        const std::size_t firstExtension =
            state.recentExtensions.size() > MaxSnapshotCodexExtensions ? state.recentExtensions.size() - MaxSnapshotCodexExtensions : 0;
        snapshot.omittedRecentExtensions = firstExtension;
        snapshot.recentExtensions.reserve(state.recentExtensions.size() - firstExtension);
        for (std::size_t index = firstExtension; index < state.recentExtensions.size(); ++index) {
            snapshot.recentExtensions.push_back(makeExtensionSnapshot(state.recentExtensions[index]));
        }
        return snapshot;
    }

} // namespace ai::openai::codex::backend
