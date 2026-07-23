#include "tests/policy/SourcePolicyTestRoot.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    struct Token {
        std::string text;
        std::size_t position;
    };

    struct Call {
        std::string path;
        std::string helper;
        std::size_t position;
    };

    struct AllowEntry {
        std::string path;
        std::string helper;
        std::string marker;
        std::string classification;
        std::string reason;
    };

    using SourceMap = std::map<std::string, std::string>;

    constexpr std::string_view kReducerPath = "src/ai/openai/codex/backend/Reducer.cpp";

    bool isIdentifierCharacter(char character) {
        const unsigned char value = static_cast<unsigned char>(character);
        return std::isalnum(value) != 0 || character == '_';
    }

    void maskRange(std::string& masked, std::size_t begin, std::size_t end) {
        for (std::size_t position = begin; position < end; ++position) {
            if (masked[position] != '\n' && masked[position] != '\r') {
                masked[position] = ' ';
            }
        }
    }

    void maskLiteral(std::string& masked, std::size_t begin, std::size_t end) {
        maskRange(masked, begin, end);
        if (begin < end) {
            masked[begin] = '@';
        }
    }

    std::size_t prefixedLiteralQuote(std::string_view source, std::size_t position, char quote) {
        if (source[position] == quote) {
            return position;
        }
        for (const std::string_view prefix : {std::string_view{"u8"}, std::string_view{"u"}, std::string_view{"U"}, std::string_view{"L"}}) {
            if (source.substr(position).starts_with(prefix) && position + prefix.size() < source.size() &&
                source[position + prefix.size()] == quote) {
                return position + prefix.size();
            }
        }
        return std::string_view::npos;
    }

    std::pair<std::size_t, std::size_t> rawLiteralBounds(std::string_view source, std::size_t position) {
        for (const std::string_view prefix : {std::string_view{"R"},
                                              std::string_view{"u8R"},
                                              std::string_view{"uR"},
                                              std::string_view{"UR"},
                                              std::string_view{"LR"}}) {
            if (!source.substr(position).starts_with(prefix) || position + prefix.size() >= source.size() ||
                source[position + prefix.size()] != '"') {
                continue;
            }

            const std::size_t delimiterBegin = position + prefix.size() + 1;
            const std::size_t open = source.find('(', delimiterBegin);
            if (open == std::string_view::npos || open - delimiterBegin > 16) {
                return {position, source.size()};
            }
            const std::string delimiter(source.substr(delimiterBegin, open - delimiterBegin));
            const std::string close = ")" + delimiter + "\"";
            const std::size_t closePosition = source.find(close, open + 1);
            return {position, closePosition == std::string_view::npos ? source.size() : closePosition + close.size()};
        }
        return {std::string_view::npos, std::string_view::npos};
    }

    std::string maskCommentsAndLiterals(std::string_view source) {
        std::string masked(source);
        std::size_t position = 0;
        while (position < source.size()) {
            if (source.substr(position).starts_with("//")) {
                const std::size_t end = source.find('\n', position + 2);
                const std::size_t maskedEnd = end == std::string_view::npos ? source.size() : end;
                maskRange(masked, position, maskedEnd);
                position = maskedEnd;
                continue;
            }
            if (source.substr(position).starts_with("/*")) {
                const std::size_t end = source.find("*/", position + 2);
                const std::size_t maskedEnd = end == std::string_view::npos ? source.size() : end + 2;
                maskRange(masked, position, maskedEnd);
                position = maskedEnd;
                continue;
            }

            const bool prefixBoundary = position == 0 || !isIdentifierCharacter(source[position - 1]);
            if (prefixBoundary) {
                const auto [rawBegin, rawEnd] = rawLiteralBounds(source, position);
                if (rawBegin != std::string_view::npos) {
                    maskLiteral(masked, rawBegin, rawEnd);
                    position = rawEnd;
                    continue;
                }

                std::size_t quotePosition = prefixedLiteralQuote(source, position, '"');
                if (quotePosition == std::string_view::npos) {
                    quotePosition = prefixedLiteralQuote(source, position, '\'');
                }
                if (quotePosition != std::string_view::npos) {
                    const char quote = source[quotePosition];
                    std::size_t end = quotePosition + 1;
                    while (end < source.size()) {
                        if (source[end] == '\\') {
                            end = std::min(source.size(), end + 2);
                        } else if (source[end++] == quote) {
                            break;
                        }
                    }
                    maskLiteral(masked, position, end);
                    position = end;
                    continue;
                }
            }
            ++position;
        }
        return masked;
    }

    std::vector<Token> tokenize(std::string_view source) {
        std::vector<Token> tokens;
        for (std::size_t position = 0; position < source.size();) {
            if (isIdentifierCharacter(source[position]) &&
                std::isdigit(static_cast<unsigned char>(source[position])) == 0) {
                const std::size_t begin = position++;
                while (position < source.size() && isIdentifierCharacter(source[position])) {
                    ++position;
                }
                tokens.push_back({std::string(source.substr(begin, position - begin)), begin});
            } else if (source.substr(position).starts_with("::")) {
                tokens.push_back({"::", position});
                position += 2;
            } else if (std::isspace(static_cast<unsigned char>(source[position])) != 0) {
                ++position;
            } else {
                tokens.push_back({std::string(1, source[position]), position});
                ++position;
            }
        }
        return tokens;
    }

    bool endsWithLog(std::string_view helper) {
        return helper.size() >= 3 && helper.ends_with("Log");
    }

    std::vector<Call> findParameterlessCalls(std::string_view path, std::string_view source) {
        const std::vector<Token> tokens = tokenize(maskCommentsAndLiterals(source));
        std::vector<Call> calls;
        for (std::size_t index = 0; index + 4 < tokens.size(); ++index) {
            if (tokens[index].text == "semantic" && tokens[index + 1].text == "::" && endsWithLog(tokens[index + 2].text) &&
                tokens[index + 3].text == "(" && tokens[index + 4].text == ")") {
                calls.push_back({std::string(path), tokens[index + 2].text, tokens[index].position});
                continue;
            }

            if (path == kReducerPath && tokens[index].text == "lifecycleLog" && tokens[index + 1].text == "(" &&
                tokens[index + 2].text == ")" && tokens[index + 3].text == ".") {
                calls.push_back({std::string(path), "lifecycleLog", tokens[index].position});
            }
        }
        return calls;
    }

    std::size_t occurrenceCount(std::string_view source, std::string_view marker) {
        std::size_t count = 0;
        for (std::size_t position = 0; (position = source.find(marker, position)) != std::string_view::npos; position += marker.size()) {
            ++count;
        }
        return count;
    }

    std::string entryIdentity(const AllowEntry& entry) {
        return entry.path + '|' + entry.helper + '|' + entry.marker;
    }

    bool blank(std::string_view value) {
        return value.empty() || std::ranges::all_of(value, [](unsigned char character) { return std::isspace(character) != 0; });
    }

    bool validateAllowlist(const SourceMap& sources,
                           const std::vector<Call>& calls,
                           const std::vector<AllowEntry>& allowlist,
                           bool diagnostics) {
        static const std::set<std::string> validClassifications = {"PREBOUND_FALLBACK",
                                                                   "POSTDISCONNECT_FALLBACK",
                                                                   "GLOBAL_COMPONENT_DIAGNOSTIC",
                                                                   "DOMAIN_OR_PROTOCOL_SCOPE",
                                                                   "FROZEN_PHASE1_PHASE2",
                                                                   "APPLICATION_FACING",
                                                                   "HELPER_FALLBACK_IMPLEMENTATION",
                                                                   "UNRESOLVED_STATIC_BUT_BINDABLE"};

        auto report = [diagnostics](const std::string& message) {
            if (diagnostics) {
                std::cerr << message << '\n';
            }
        };

        bool ok = true;
        std::set<std::string> identities;
        std::vector<std::size_t> matchedBy(calls.size(), 0);
        std::vector<std::vector<std::string>> matchingEntries(calls.size());
        for (const AllowEntry& entry : allowlist) {
            const std::string identity = entryIdentity(entry);
            if (!identities.insert(identity).second) {
                report("Duplicate Phase 3 parameterless-helper allowlist identity: " + identity);
                ok = false;
            }
            if (!validClassifications.contains(entry.classification) || blank(entry.classification) || blank(entry.reason)) {
                report("Invalid or incomplete Phase 3 parameterless-helper allowlist entry: " + identity);
                ok = false;
            }
            if (entry.classification == "UNRESOLVED_STATIC_BUT_BINDABLE") {
                report("Unresolved static-but-bindable Phase 3 logger remains: " + identity);
                ok = false;
            }

            const auto source = sources.find(entry.path);
            if (source == sources.end()) {
                report("Stale Phase 3 parameterless-helper path: " + entry.path);
                ok = false;
                continue;
            }
            if (entry.marker.empty() || occurrenceCount(source->second, entry.marker) != 1) {
                report("Phase 3 parameterless-helper marker is missing or non-unique: " + identity);
                ok = false;
                continue;
            }

            const std::size_t markerPosition = source->second.find(entry.marker);
            std::size_t bestCall = std::numeric_limits<std::size_t>::max();
            std::size_t bestDistance = std::numeric_limits<std::size_t>::max();
            bool tied = false;
            for (std::size_t index = 0; index < calls.size(); ++index) {
                if (calls[index].path != entry.path || calls[index].helper != entry.helper) {
                    continue;
                }
                const std::size_t distance = calls[index].position > markerPosition ? calls[index].position - markerPosition
                                                                                    : markerPosition - calls[index].position;
                if (distance < bestDistance) {
                    bestCall = index;
                    bestDistance = distance;
                    tied = false;
                } else if (distance == bestDistance) {
                    tied = true;
                }
            }
            if (bestCall == std::numeric_limits<std::size_t>::max() || tied || bestDistance > 2048) {
                report("Stale Phase 3 parameterless-helper allowlist entry: " + identity);
                ok = false;
                continue;
            }
            ++matchedBy[bestCall];
            matchingEntries[bestCall].push_back(identity);
        }

        for (std::size_t index = 0; index < calls.size(); ++index) {
            const Call& call = calls[index];
            if (matchedBy[index] == 0) {
                report("Unexpected unclassified parameterless semantic helper: " + call.path + '|' + call.helper + "@" +
                       std::to_string(call.position));
                ok = false;
            } else if (matchedBy[index] != 1) {
                report("Multiple allowlist entries resolve to one parameterless semantic helper: " + call.path + '|' + call.helper + "@" +
                       std::to_string(call.position));
                for (const std::string& identity : matchingEntries[index]) {
                    report("  " + identity);
                }
                ok = false;
            }
        }
        return ok && calls.size() == allowlist.size();
    }

    std::vector<AllowEntry> parameterlessAllowlist() {
        using Entry = AllowEntry;
        return {
            // Process-wide HTTP upgrade/MIME selectors.
            Entry{"src/web/http/SocketContextUpgradeFactorySelector.hpp", "webHttpLog", "SocketContextUpgradeFactory create success:",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "Dynamic HTTP upgrade factory selector has no connection owner."},
            Entry{"src/web/http/SocketContextUpgradeFactorySelector.hpp", "webHttpLog", "SocketContextUpgradeFactory already existing:",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "Dynamic HTTP upgrade factory cache diagnostic is process-wide."},
            Entry{"src/web/http/SocketContextUpgradeFactorySelector.hpp", "webHttpLog", "SocketContextUpgradeFactory create failed:",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "Dynamic HTTP upgrade factory load failure is process-wide."},
            Entry{"src/web/http/SocketContextUpgradeFactorySelector.hpp", "webHttpLog", "Optaining function",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "Dynamic HTTP upgrade symbol lookup is process-wide."},
            Entry{"src/web/http/SocketContextUpgradeFactorySelector.hpp", "webHttpLog", "selected from dynamic cache",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "HTTP upgrade plugin selection cache is process-wide."},
            Entry{"src/web/http/SocketContextUpgradeFactorySelector.hpp", "webHttpLog", "selected from static cache",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "HTTP upgrade linked-plugin selection is process-wide."},
            Entry{"src/web/http/SocketContextUpgradeFactorySelector.hpp", "webHttpLog",
                  "socketContextUpgradeFactory = load(socketContextUpgradeName);",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "HTTP upgrade dynamic cache mutation is process-wide."},
            Entry{"src/web/http/SocketContextUpgradeFactorySelector.hpp", "webHttpLog", "not found",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "HTTP upgrade missing-plugin decision is process-wide."},
            Entry{"src/web/http/MimeTypes.cpp", "webHttpLog", "Cannot load magic database",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "MIME database initialization belongs to the global HTTP component."},
            Entry{"src/web/http/client/SocketContextUpgradeFactorySelector.cpp", "httpClientUpgradeLog",
                  "Overriding http upgrade library dir", "GLOBAL_COMPONENT_DIAGNOSTIC",
                  "HTTP client upgrade library configuration precedes connection selection."},
            Entry{"src/web/http/server/SocketContextUpgradeFactorySelector.cpp", "httpServerUpgradeLog",
                  "Overriding http upgrade library dir", "GLOBAL_COMPONENT_DIAGNOSTIC",
                  "HTTP server upgrade library configuration precedes connection selection."},

            // EventSource URL validation runs before a Client or SocketConnection exists.
            Entry{"src/web/http/legacy/rc/EventSource.h", "httpClientEventSourceLog", "RFCOMM url not accepted:",
                  "PREBOUND_FALLBACK", "Legacy RFCOMM EventSource URL validation has no connection yet."},
            Entry{"src/web/http/legacy/in/EventSource.h", "httpClientEventSourceLog", "Scheme not valid:", "PREBOUND_FALLBACK",
                  "Legacy IPv4 EventSource scheme validation has no connection yet."},
            Entry{"src/web/http/legacy/in/EventSource.h", "httpClientEventSourceLog", "url not accepted:", "PREBOUND_FALLBACK",
                  "Legacy IPv4 EventSource URL validation has no connection yet."},
            Entry{"src/web/http/legacy/in6/EventSource.h", "httpClientEventSourceLog", "Scheme not valid:", "PREBOUND_FALLBACK",
                  "Legacy IPv6 EventSource scheme validation has no connection yet."},
            Entry{"src/web/http/legacy/in6/EventSource.h", "httpClientEventSourceLog", "url not accepted:", "PREBOUND_FALLBACK",
                  "Legacy IPv6 EventSource URL validation has no connection yet."},
            Entry{"src/web/http/legacy/un/EventSource.h", "httpClientEventSourceLog", "UNIX socket must decode to absolute",
                  "PREBOUND_FALLBACK", "Legacy Unix EventSource socket-path validation has no connection yet."},
            Entry{"src/web/http/legacy/un/EventSource.h", "httpClientEventSourceLog", "unix-domain url not accepted:",
                  "PREBOUND_FALLBACK", "Legacy Unix EventSource URL validation has no connection yet."},
            Entry{"src/web/http/tls/rc/EventSource.h", "httpClientEventSourceLog", "RFCOMM url not accepted:",
                  "PREBOUND_FALLBACK", "TLS RFCOMM EventSource URL validation has no connection yet."},
            Entry{"src/web/http/tls/in/EventSource.h", "httpClientEventSourceLog", "Scheme not valid:", "PREBOUND_FALLBACK",
                  "TLS IPv4 EventSource scheme validation has no connection yet."},
            Entry{"src/web/http/tls/in/EventSource.h", "httpClientEventSourceLog", "url not accepted:", "PREBOUND_FALLBACK",
                  "TLS IPv4 EventSource URL validation has no connection yet."},
            Entry{"src/web/http/tls/in6/EventSource.h", "httpClientEventSourceLog", "Scheme not valid:", "PREBOUND_FALLBACK",
                  "TLS IPv6 EventSource scheme validation has no connection yet."},
            Entry{"src/web/http/tls/in6/EventSource.h", "httpClientEventSourceLog", "url not accepted:", "PREBOUND_FALLBACK",
                  "TLS IPv6 EventSource URL validation has no connection yet."},
            Entry{"src/web/http/tls/un/EventSource.h", "httpClientEventSourceLog", "UNIX socket must decode to absolute",
                  "PREBOUND_FALLBACK", "TLS Unix EventSource socket-path validation has no connection yet."},
            Entry{"src/web/http/tls/un/EventSource.h", "httpClientEventSourceLog", "unix-domain url not accepted:",
                  "PREBOUND_FALLBACK", "TLS Unix EventSource URL validation has no connection yet."},

            // EventSource owns a protocol lifecycle that spans transport connections.
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientLog", "Origin: ", "PREBOUND_FALLBACK",
                  "EventSource origin is configured before its first connection."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientLog", "  Path: ", "PREBOUND_FALLBACK",
                  "EventSource path is configured before its first connection."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientEventSourceLog", "event source reconnect scheduled:",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "SSE retry scheduling belongs to the long-lived EventSource protocol."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientEventSourceLog", "event source reconnect dispatched:",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "SSE retry dispatch belongs to the long-lived EventSource protocol."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientEventSourceLog", "event stream established:",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Stream establishment belongs to EventSource rather than one transport."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientLog", "req->getConnectionName()", "POSTDISCONNECT_FALLBACK",
                  "Weak EventSource owner is gone when OnRequestEnd falls back to request display identity."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientEventSourceLog", "event source started:",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "EventSource start spans zero or more transport connections."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientLog", "case core::socket::State::OK:", "FROZEN_PHASE1_PHASE2",
                  "Connector OK callback wording remains part of the accepted Phase 2 surface."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientLog", "case core::socket::State::DISABLED:",
                  "FROZEN_PHASE1_PHASE2", "Connector disabled callback wording remains part of the accepted Phase 2 surface."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientLog", "case core::socket::State::ERROR:", "FROZEN_PHASE1_PHASE2",
                  "Connector error callback wording remains part of the accepted Phase 2 surface."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientLog", "case core::socket::State::FATAL:", "FROZEN_PHASE1_PHASE2",
                  "Connector fatal callback wording remains part of the accepted Phase 2 surface."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientEventSourceLog", "event source reconnect cancelled:",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "SSE retry cancellation belongs to the long-lived EventSource protocol."},
            Entry{"src/web/http/client/tools/EventSource.h", "httpClientEventSourceLog", "event source stopped:",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "EventSource stop spans zero or more transport connections."},
            Entry{"src/web/http/server/Response.cpp", "httpServerLog", "Unexpected disconnect during upgrade",
                  "POSTDISCONNECT_FALLBACK", "Response::disconnect clears SocketContext, so no live connection is safe here."},

            // WebSocket global selectors and connection-independent frame codec scopes.
            Entry{"src/web/websocket/SubProtocolFactorySelector.hpp", "webSocketFactoryLog", "SubProtocolFactory create success:",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "WebSocket subprotocol factory creation is process-wide."},
            Entry{"src/web/websocket/SubProtocolFactorySelector.hpp", "webSocketFactoryLog", "SubProtocolFactory create failed:",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "WebSocket subprotocol factory failure is process-wide."},
            Entry{"src/web/websocket/SubProtocolFactorySelector.hpp", "webSocketFactoryLog", "Optaining function",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "WebSocket subprotocol symbol lookup is process-wide."},
            Entry{"src/web/websocket/SubProtocolFactorySelector.hpp", "webSocketFactoryLog", "selected from dynamic cache",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "WebSocket subprotocol dynamic-cache selection is process-wide."},
            Entry{"src/web/websocket/SubProtocolFactorySelector.hpp", "webSocketFactoryLog", "selected from static cache",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "WebSocket subprotocol linked-plugin selection is process-wide."},
            Entry{"src/web/websocket/SubProtocolFactorySelector.hpp", "webSocketFactoryLog",
                  "subProtocolFactories.insert({subProtocolName, subProtocolFactory});",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "WebSocket subprotocol dynamic cache mutation is process-wide."},
            Entry{"src/web/websocket/SubProtocolFactorySelector.hpp", "webSocketFactoryLog", "not found",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "WebSocket missing-subprotocol decision is process-wide."},
            Entry{"src/web/websocket/client/SubProtocolFactorySelector.cpp", "webSocketFactoryLog",
                  "Overriding websocket subprotocol library dir", "GLOBAL_COMPONENT_DIAGNOSTIC",
                  "WebSocket client subprotocol library configuration is process-wide."},
            Entry{"src/web/websocket/server/SubProtocolFactorySelector.cpp", "webSocketFactoryLog",
                  "Overriding websocket subprotocol library dir", "GLOBAL_COMPONENT_DIAGNOSTIC",
                  "WebSocket server subprotocol library configuration is process-wide."},
            Entry{"src/web/websocket/Receiver.cpp", "webSocketFrameLog", "Receiver::frameLog() const", "DOMAIN_OR_PROTOCOL_SCOPE",
                  "Receiver frame diagnostics belong to a reusable codec without a SocketConnection owner."},
            Entry{"src/web/websocket/Transmitter.cpp", "webSocketFrameLog", "Transmitter::frameLog() const",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Transmitter frame diagnostics belong to a reusable codec without a SocketConnection owner."},

            // MQTT helper fallbacks, pre/post-attachment persistence, and process-wide broker state.
            Entry{"src/iot/mqtt/Mqtt.cpp", "mqttLog", "log_.emplace(iot::mqtt::semantic::mqttLog());",
                  "HELPER_FALLBACK_IMPLEMENTATION", "Cached common logger falls back only when MqttContext has no live connection."},
            Entry{"src/iot/mqtt/client/Mqtt.cpp", "mqttClientLog", "return iot::mqtt::semantic::mqttClientLog();",
                  "HELPER_FALLBACK_IMPLEMENTATION", "Local clientLog helper falls back only without a live MqttContext connection."},
            Entry{"src/iot/mqtt/client/Mqtt.cpp", "mqttClientLog", "Persistent session data loaded successful", "PREBOUND_FALLBACK",
                  "Client constructor loads session storage before MqttContext attachment."},
            Entry{"src/iot/mqtt/client/Mqtt.cpp", "mqttClientLog", "Starting with empty session:", "PREBOUND_FALLBACK",
                  "Client constructor diagnoses corrupt session storage before attachment."},
            Entry{"src/iot/mqtt/client/Mqtt.cpp", "mqttClientLog", "Restoring saved session done", "PREBOUND_FALLBACK",
                  "Client constructor completes session restore before attachment."},
            Entry{"src/iot/mqtt/client/Mqtt.cpp", "mqttClientLog", "Could not read session store", "PREBOUND_FALLBACK",
                  "Client constructor cannot bind session-store read failure to a connection."},
            Entry{"src/iot/mqtt/client/Mqtt.cpp", "mqttClientLog", "Session not reloaded:", "PREBOUND_FALLBACK",
                  "Client constructor has no MqttContext when session restore is disabled."},
            Entry{"src/iot/mqtt/client/Mqtt.cpp", "mqttClientLog", "Could not write session store", "POSTDISCONNECT_FALLBACK",
                  "Client destructor writes session storage after connection ownership is unsafe."},
            Entry{"src/iot/mqtt/client/Mqtt.cpp", "mqttClientLog", "Session not saved:", "POSTDISCONNECT_FALLBACK",
                  "Client destructor reports disabled persistence after connection ownership is unsafe."},
            Entry{"src/iot/mqtt/server/Mqtt.cpp", "mqttServerLog", "return iot::mqtt::semantic::mqttServerLog();",
                  "HELPER_FALLBACK_IMPLEMENTATION", "Local serverLog helper falls back only without a live MqttContext connection."},
            Entry{"src/iot/mqtt/server/Mqtt.cpp", "mqttServerLog", "releaseSession(iot::mqtt::semantic::mqttServerLog()",
                  "POSTDISCONNECT_FALLBACK", "Server destructor releases broker session after safe connection scope has ended."},
            Entry{"src/iot/mqtt/server/broker/Broker.cpp", "mqttBrokerLog", "Broker::log() const",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "Broker persistence and routing state is process-wide rather than connection-owned."},
            Entry{"src/iot/mqtt/server/broker/RetainTree.cpp", "mqttBrokerLog", "RetainTree::log() const",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "Retained-message root belongs to the process-wide broker."},
            Entry{"src/iot/mqtt/server/broker/RetainTree.cpp", "mqttBrokerLog", "RetainTree::TopicLevel::log() const",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "Retained-message topic level belongs to the process-wide broker."},
            Entry{"src/iot/mqtt/server/broker/Session.cpp", "mqttSessionLog", "Session::log() const",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Persistent broker session may outlive any one transport connection."},
            Entry{"src/iot/mqtt/server/broker/SubscriptionTree.cpp", "mqttBrokerLog", "SubscriptionTree::log() const",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "Subscription root belongs to the process-wide broker."},
            Entry{"src/iot/mqtt/server/broker/SubscriptionTree.cpp", "mqttBrokerLog", "SubscriptionTree::TopicLevel::log() const",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "Subscription topic level belongs to the process-wide broker."},

            // Codex reducer events and MariaDB operations have protocol/domain identity, not SocketConnection identity.
            Entry{"src/ai/openai/codex/backend/Reducer.cpp", "lifecycleLog", "turn {}: thread={} turn={}",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Typed turn completion is owned by Codex thread and turn identifiers."},
            Entry{"src/ai/openai/codex/backend/Reducer.cpp", "lifecycleLog", "turn failed: thread={} turn={}",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Typed turn failure is owned by Codex thread and turn identifiers."},
            Entry{"src/ai/openai/codex/backend/Reducer.cpp", "lifecycleLog", "thread created: thread={}",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Typed thread creation is owned by the Codex thread identifier."},
            Entry{"src/ai/openai/codex/backend/Reducer.cpp", "lifecycleLog", "turn started: thread={} turn={}",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Typed turn start is owned by Codex thread and turn identifiers."},
            Entry{"src/database/mariadb/MariaDBLibrary.cpp", "mariaDbLog", "mysql_library_init failed",
                  "GLOBAL_COMPONENT_DIAGNOSTIC", "MariaDB library initialization is a process-wide component diagnostic."},
            Entry{"src/database/mariadb/MariaDBConnection.cpp", "mariaDbLog", "Descriptor not registered in SNode.C eventloop",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "MariaDB connection name is the available database-domain identity."},
            Entry{"src/database/mariadb/MariaDBConnection.cpp", "mariaDbLog", "connect: success", "DOMAIN_OR_PROTOCOL_SCOPE",
                  "MariaDB connection has no SocketConnection semantic scope."},
            Entry{"src/database/mariadb/MariaDBConnection.cpp", "mariaDbLog", "database session established:",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Database session identity is carried by the MariaDB connection name."},
            Entry{"src/database/mariadb/MariaDBConnection.cpp", "mariaDbLog", "connect: error:", "DOMAIN_OR_PROTOCOL_SCOPE",
                  "MariaDB connection failure has no SocketConnection semantic scope."},
            Entry{"src/database/mariadb/MariaDBConnection.cpp", "mariaDbLog", "closing || core::SNodeC::state()",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Destructor request terminal belongs to the MariaDB command domain."},
            Entry{"src/database/mariadb/MariaDBConnection.cpp", "mariaDbLog", "database session ended:",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Database session identity is carried by the MariaDB connection name."},
            Entry{"src/database/mariadb/MariaDBConnection.cpp", "mariaDbLog", "database request started:",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Database request identity is the MariaDB command and connection name."},
            Entry{"src/database/mariadb/MariaDBConnection.cpp", "mariaDbLog", "currentCommandFailed ? \"failed\" : \"completed\"",
                  "DOMAIN_OR_PROTOCOL_SCOPE", "Database request terminal is owned by the active MariaDB command."},
            Entry{"src/database/mariadb/MariaDBConnection.cpp", "mariaDbLog", "Lost connection", "DOMAIN_OR_PROTOCOL_SCOPE",
                  "MariaDB has no SocketConnection logger and retains its database connection identity."},
        };
    }

    SourceMap readParameterlessSources(const std::filesystem::path& root) {
        SourceMap sources;
        for (const std::filesystem::path relativeRoot : {std::filesystem::path{"src/web/http"},
                                                         std::filesystem::path{"src/web/websocket"},
                                                         std::filesystem::path{"src/iot/mqtt"},
                                                         std::filesystem::path{"src/ai/openai/codex"},
                                                         std::filesystem::path{"src/database/mariadb"}}) {
            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(root / relativeRoot)) {
                if (!entry.is_regular_file()) {
                    continue;
                }
                const std::string extension = entry.path().extension().string();
                if (extension != ".cpp" && extension != ".h" && extension != ".hpp" && extension != ".ipp") {
                    continue;
                }
                const std::string relative = std::filesystem::relative(entry.path(), root).generic_string();
                sources.emplace(relative, source_policy::readSourcePolicyFile(entry.path()));
            }
        }
        return sources;
    }

    std::vector<Call> discover(const SourceMap& sources) {
        std::vector<Call> calls;
        for (const auto& [path, source] : sources) {
            std::vector<Call> fileCalls = findParameterlessCalls(path, source);
            calls.insert(calls.end(), fileCalls.begin(), fileCalls.end());
        }
        return calls;
    }

    bool scannerSelfTest() {
        const std::string source = R"cpp(
            semantic::alphaLog /* permitted formatting */ ( /* empty */ );
            semantic::alphaLog(value);
            semantic::alphaLog("argument");
            object.log();
            object.frameworkLog();
            // semantic::commentLog()
            const char* text = "semantic::stringLog()";
            const char* raw = R"tag(semantic::rawStringLog())tag";
            const char character = ')';
        )cpp";
        const std::vector<Call> calls = findParameterlessCalls("src/example.cpp", source);
        if (calls.size() != 1 || calls.front().helper != "alphaLog") {
            std::cerr << "Scanner self-test discovered " << calls.size() << " calls";
            for (const Call& call : calls) {
                std::cerr << ' ' << call.helper;
            }
            std::cerr << '\n';
            return false;
        }
        return true;
    }

    bool guardSelfTest() {
        const SourceMap sources{{"src/example.cpp", "void owner_marker() { semantic::alphaLog /* gap */ ( ); }"}};
        const std::vector<Call> calls = discover(sources);
        const AllowEntry valid{"src/example.cpp", "alphaLog", "owner_marker", "GLOBAL_COMPONENT_DIAGNOSTIC", "test reason"};
        if (!validateAllowlist(sources, calls, {valid}, false)) {
            std::cerr << "Guard self-test rejected its valid allowlist\n";
            return false;
        }
        if (validateAllowlist(sources, calls, {}, false)) { // injected unexpected call
            std::cerr << "Guard self-test accepted an unexpected call\n";
            return false;
        }
        if (validateAllowlist(sources,
                              calls,
                              {{"src/example.cpp", "betaLog", "owner_marker", "GLOBAL_COMPONENT_DIAGNOSTIC", "stale"}},
                              false)) {
            std::cerr << "Guard self-test accepted a stale entry\n";
            return false;
        }
        if (validateAllowlist(sources, calls, {valid, valid}, false)) {
            std::cerr << "Guard self-test accepted duplicate entries\n";
            return false;
        }
        if (validateAllowlist(sources,
                              calls,
                              {{"src/example.cpp", "alphaLog", "owner_marker", " ", "missing classification"}},
                              false) ||
            validateAllowlist(sources,
                              calls,
                              {{"src/example.cpp", "alphaLog", "owner_marker", "GLOBAL_COMPONENT_DIAGNOSTIC", " \t"}},
                              false) ||
            validateAllowlist(sources,
                              calls,
                              {{"src/example.cpp", "alphaLog", "owner_marker", "UNRESOLVED_STATIC_BUT_BINDABLE", "unresolved"}},
                              false)) {
            std::cerr << "Guard self-test accepted incomplete or unresolved entries\n";
            return false;
        }
        const SourceMap removed{{"src/example.cpp", "void owner_marker() {}"}};
        if (validateAllowlist(removed, discover(removed), {valid}, false)) {
            std::cerr << "Guard self-test accepted a disappeared call\n";
            return false;
        }
        return true;
    }
} // namespace

int main() {
    if (!scannerSelfTest() || !guardSelfTest()) {
        std::cerr << "Phase 3 parameterless semantic logger policy self-test failed\n";
        return 1;
    }

    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }
    const SourceMap sources = readParameterlessSources(root);
    const std::vector<Call> calls = discover(sources);
    const std::vector<AllowEntry> allowlist = parameterlessAllowlist();
    if (calls.size() != 81 || allowlist.size() != 81) {
        std::cerr << "Phase 3 parameterless semantic logger accounting changed: discovered=" << calls.size()
                  << " allowlisted=" << allowlist.size() << " expected=81\n";
        return 1;
    }
    return validateAllowlist(sources, calls, allowlist, true) ? 0 : 1;
}
