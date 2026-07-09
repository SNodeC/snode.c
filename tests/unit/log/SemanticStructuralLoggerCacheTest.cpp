#include "core/EventLoop.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"
#include "tests/unit/log/SourcePolicyTestRoot.h"

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace {
    struct ExpensiveArgument {
        int* counter;
    };

    std::ostream& operator<<(std::ostream& os, const ExpensiveArgument& value) {
        ++*value.counter;
        return os << "expensive";
    }

    class SemanticStateGuard {
    public:
        SemanticStateGuard() {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
        }

        ~SemanticStateGuard() {
            logger::Logger::init();
            logger::LogManager::init();
        }
    };

    bool containsAny(std::string_view source, std::initializer_list<std::string_view> needles) {
        for (const auto needle : needles) {
            if (source_policy::contains(source, needle)) {
                return true;
            }
        }
        return false;
    }

    bool containsDirectWebSocketFrameHotCall(std::string_view source) {
        return source_policy::contains(source, "auto log = semantic::webSocketFrameLog()") ||
               source_policy::contains(source, "semantic::webSocketFrameLog().enabled") ||
               source_policy::contains(source, "semantic::webSocketFrameLog().trace");
    }

    bool containsDirectMqttBrokerOrSessionSeverity(std::string_view source) {
        constexpr std::string_view prefixes[] = {"iot::mqtt::semantic::mqttBrokerLog().", "iot::mqtt::semantic::mqttSessionLog()."};
        constexpr std::string_view severities[] = {"trace", "debug", "info", "warn", "error", "critical", "enabled", "sysError"};
        for (const auto prefix : prefixes) {
            for (const auto severity : severities) {
                if (source_policy::contains(source, std::string(prefix) + std::string(severity))) {
                    return true;
                }
            }
        }
        return false;
    }

    bool isDeclaredAfterNearestPrivateSpecifier(std::string_view source, std::size_t declarationPos) {
        enum class AccessSpecifier { None, Public, Protected, Private };

        std::size_t nearestPos = 0;
        AccessSpecifier nearestAccessSpecifier = AccessSpecifier::None;

        const auto consider = [&](std::string_view specifier, AccessSpecifier accessSpecifier) {
            const std::size_t pos = source.rfind(specifier, declarationPos);
            if (pos != std::string_view::npos && (nearestAccessSpecifier == AccessSpecifier::None || pos > nearestPos)) {
                nearestPos = pos;
                nearestAccessSpecifier = accessSpecifier;
            }
        };

        consider("public:", AccessSpecifier::Public);
        consider("protected:", AccessSpecifier::Protected);
        consider("private:", AccessSpecifier::Private);

        return nearestAccessSpecifier == AccessSpecifier::Private;
    }

    bool isDeclaredAfterNearestPrivateSpecifier(std::string_view source, std::string_view declaration) {
        const std::size_t declarationPos = source.find(declaration);
        return declarationPos != std::string_view::npos && isDeclaredAfterNearestPrivateSpecifier(source, declarationPos);
    }

    bool allDeclarationsAfterNearestPrivateSpecifier(std::string_view source, std::string_view declaration, std::size_t expectedCount) {
        std::size_t count = 0;
        std::size_t searchPos = 0;
        while (true) {
            const std::size_t declarationPos = source.find(declaration, searchPos);
            if (declarationPos == std::string_view::npos) {
                break;
            }
            ++count;
            if (!isDeclaredAfterNearestPrivateSpecifier(source, declarationPos)) {
                return false;
            }
            searchPos = declarationPos + declaration.size();
        }

        return count == expectedCount;
    }
} // namespace

int main() {
    tests::support::TestResult result;

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
        logger::LogManager::freeze();

        logger::LogScopeOwner scope(logger::LogOrigin::Framework,
                                    logger::LogBoundary::Connection,
                                    "core.socket.stream",
                                    "unit-instance",
                                    std::nullopt,
                                    "unit-connection");
        const auto cachedLog = scope.logger(logger::Logger::semanticSink());
        result.expectTrue(!cachedLog.enabled(logger::LogLevel::Debug), "cached logger honors global threshold");

        int counter = 0;
        cachedLog.debug("value={}", ExpensiveArgument{&counter});
        result.expectEqual(0, counter, "disabled cached logger does not format expensive arguments");
    }

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Warn);
        logger::LogManager::setComponentLevel("core.socket.stream", logger::LogLevel::Debug);
        logger::LogManager::freeze();

        logger::LogScopeOwner scope(logger::LogOrigin::Framework,
                                    logger::LogBoundary::Connection,
                                    "core.socket.stream",
                                    std::nullopt,
                                    std::nullopt,
                                    "unit-connection");
        const auto cachedLog = scope.logger(logger::Logger::semanticSink());
        result.expectTrue(cachedLog.enabled(logger::LogLevel::Debug), "cached logger honors component override");

        int counter = 0;
        cachedLog.debug("value={}", ExpensiveArgument{&counter});
        result.expectEqual(1, counter, "enabled cached logger formats expensive arguments exactly once");
    }

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Warn);
        logger::LogManager::setInstanceLevel("unit-instance", logger::LogLevel::Trace);
        logger::LogManager::freeze();

        logger::LogScopeOwner scope(logger::LogOrigin::Framework,
                                    logger::LogBoundary::Context,
                                    "core.socket.context",
                                    "unit-instance",
                                    std::nullopt,
                                    "unit-connection");
        const auto cachedLog = scope.logger(logger::Logger::semanticSink());
        result.expectTrue(cachedLog.enabled(logger::LogLevel::Trace), "cached logger honors instance override");
    }

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
        logger::LogManager::freeze();
        result.expectTrue(!core::EventLoop::instance().log().enabled(logger::LogLevel::Debug),
                          "EventLoop cached logger honors initial info generation");

        logger::LogManager::init();
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::freeze();
        result.expectTrue(core::EventLoop::instance().log().enabled(logger::LogLevel::Debug),
                          "EventLoop cached logger refreshes after LogManager generation changes");
    }

    constexpr std::string_view brokerLogDeclaration = "const logger::BoundaryLogger& log() const;";

    const std::string publicAccessor = R"cpp(
class Example {
public:
    void foo();

    const logger::BoundaryLogger& log() const;

private:
    int value = 0;
};
)cpp";
    const std::string privateAccessor = R"cpp(
class Example {
public:
    void foo();

private:
    const logger::BoundaryLogger& log() const;

    int value = 0;
};
)cpp";
    result.expectTrue(!isDeclaredAfterNearestPrivateSpecifier(publicAccessor, brokerLogDeclaration),
                      "privacy helper rejects public logger accessor placement");
    result.expectTrue(isDeclaredAfterNearestPrivateSpecifier(privateAccessor, brokerLogDeclaration),
                      "privacy helper accepts private logger accessor placement");

    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        result.expectTrue(false, "sourcePolicyProjectRoot() must not return an empty path");
        return result.processResult();
    }

    // Pre-freeze-capable objects must keep an isFrozen() fallback; generation() alone only invalidates caches after LogManager::init().
    const std::string eventLoopHeader = source_policy::readSourcePolicyFile(root / "src/core/EventLoop.h");
    const std::string eventLoopSource = source_policy::readSourcePolicyFile(root / "src/core/EventLoop.cpp");
    result.expectTrue(source_policy::contains(eventLoopHeader, "std::optional<logger::BoundaryLogger> cachedLog_"),
                      "EventLoop has a lazy cached logger member");
    result.expectTrue(source_policy::contains(eventLoopSource, "logger::LogManager::isFrozen()"),
                      "EventLoop gates cached logger use on post-freeze state");
    result.expectTrue(!source_policy::contains(eventLoopSource, ", cachedLog_"),
                      "EventLoop does not initialize the semantic logger in the constructor");

    const std::string socketConnectionHeader = source_policy::readSourcePolicyFile(root / "src/core/socket/stream/SocketConnection.h");
    const std::string socketConnectionSource = source_policy::readSourcePolicyFile(root / "src/core/socket/stream/SocketConnection.cpp");
    result.expectTrue(source_policy::contains(socketConnectionHeader, "std::optional<logger::BoundaryLogger> cachedLog_"),
                      "SocketConnection has a cached logger member");
    result.expectTrue(source_policy::contains(socketConnectionSource, "if (logger::LogManager::isFrozen())"),
                      "SocketConnection uses lazy post-freeze caching");

    const std::string socketContextHeader = source_policy::readSourcePolicyFile(root / "src/core/socket/stream/SocketContext.h");
    const std::string socketContextSource = source_policy::readSourcePolicyFile(root / "src/core/socket/stream/SocketContext.cpp");
    result.expectTrue(source_policy::contains(socketContextHeader, "applicationLog_") &&
                          source_policy::contains(socketContextHeader, "frameworkLog_"),
                      "SocketContext has cached application and framework logger members");
    result.expectTrue(source_policy::contains(socketContextSource, "applicationLog_.emplace") &&
                          source_policy::contains(socketContextSource, "frameworkLog_.emplace"),
                      "SocketContext lazily populates both cached loggers");

    const std::string mqttHeader = source_policy::readSourcePolicyFile(root / "src/iot/mqtt/Mqtt.h");
    const std::string mqttSource = source_policy::readSourcePolicyFile(root / "src/iot/mqtt/Mqtt.cpp");
    result.expectTrue(source_policy::contains(mqttHeader, "std::optional<logger::BoundaryLogger> log_") &&
                          source_policy::contains(mqttHeader, "logGeneration_"),
                      "Mqtt cache is optional and generation-tracked");
    result.expectTrue(source_policy::contains(mqttSource, "logger::LogManager::generation()") &&
                          source_policy::contains(mqttSource, "log_.emplace(iot::mqtt::semantic::mqttLog())"),
                      "Mqtt cache refreshes from mqttLog when the generation changes");
    result.expectTrue(!source_policy::contains(mqttHeader, "logger::BoundaryLogger log_;"),
                      "Mqtt does not use a direct non-refreshable BoundaryLogger member");

    const std::string receiverHeader = source_policy::readSourcePolicyFile(root / "src/web/websocket/Receiver.h");
    const std::string receiverSource = source_policy::readSourcePolicyFile(root / "src/web/websocket/Receiver.cpp");
    const std::string transmitterHeader = source_policy::readSourcePolicyFile(root / "src/web/websocket/Transmitter.h");
    const std::string transmitterSource = source_policy::readSourcePolicyFile(root / "src/web/websocket/Transmitter.cpp");
    result.expectTrue(source_policy::contains(receiverHeader, "std::optional<logger::BoundaryLogger> frameLog_") &&
                          source_policy::contains(receiverHeader, "frameLogGeneration_") &&
                          source_policy::contains(transmitterHeader, "std::optional<logger::BoundaryLogger> frameLog_") &&
                          source_policy::contains(transmitterHeader, "frameLogGeneration_"),
                      "WebSocket Receiver and Transmitter use generation-tracked frame logger caches");
    result.expectTrue(source_policy::contains(receiverSource, "logger::LogManager::generation()") &&
                          source_policy::contains(transmitterSource, "logger::LogManager::generation()"),
                      "WebSocket frame caches check LogManager generation");
    result.expectTrue(!source_policy::contains(receiverHeader, "logger::BoundaryLogger frameLog_;") &&
                          !source_policy::contains(transmitterHeader, "logger::BoundaryLogger frameLog_;"),
                      "WebSocket frame caches are not direct non-refreshable BoundaryLogger members");
    result.expectTrue(!containsDirectWebSocketFrameHotCall(receiverSource) && !containsDirectWebSocketFrameHotCall(transmitterSource),
                      "WebSocket hot paths no longer call webSocketFrameLog directly");

    // Broker-family caches intentionally rely on private, enumerable runtime call sites plus generation-aware refresh.
    for (const auto [file, expectedLogAccessorCount] : {std::pair{"src/iot/mqtt/server/broker/Broker.h", std::size_t{1}},
                                                        std::pair{"src/iot/mqtt/server/broker/RetainTree.h", std::size_t{2}},
                                                        std::pair{"src/iot/mqtt/server/broker/SubscriptionTree.h", std::size_t{2}},
                                                        std::pair{"src/iot/mqtt/server/broker/Session.h", std::size_t{1}}}) {
        const std::string source = source_policy::readSourcePolicyFile(root / file);
        result.expectTrue(source_policy::contains(source, "std::optional<logger::BoundaryLogger> log_") &&
                              source_policy::contains(source, "logGeneration_") &&
                              !source_policy::contains(source, "logger::BoundaryLogger log_;"),
                          std::string(file) + " caches a generation-tracked optional BoundaryLogger");
        result.expectTrue(allDeclarationsAfterNearestPrivateSpecifier(source, brokerLogDeclaration, expectedLogAccessorCount),
                          std::string(file) + " keeps every broker-family logger accessor private");
    }
    for (const auto file : {"src/iot/mqtt/server/broker/Broker.cpp",
                            "src/iot/mqtt/server/broker/RetainTree.cpp",
                            "src/iot/mqtt/server/broker/SubscriptionTree.cpp",
                            "src/iot/mqtt/server/broker/Session.cpp"}) {
        const std::string source = source_policy::readSourcePolicyFile(root / file);
        result.expectTrue(source_policy::contains(source, "logger::LogManager::generation()"),
                          std::string(file) + " uses LogManager generation for cached logger refresh");
        result.expectTrue(!source_policy::contains(source, "const auto& log = log_"),
                          std::string(file) + " has no transitional direct-member logger aliases");
        result.expectTrue(!containsDirectMqttBrokerOrSessionSeverity(source),
                          std::string(file) + " hot diagnostics use cached MQTT loggers");
    }

    const std::string allTouchedPolicy = eventLoopHeader + eventLoopSource + socketConnectionHeader + socketConnectionSource +
                                         socketContextHeader + socketContextSource + receiverHeader + receiverSource + transmitterHeader +
                                         transmitterSource;
    result.expectTrue(!containsAny(allTouchedPolicy, {"LOG_IF", "TRACE_IF", "DEBUG_IF", "IF_ENABLED"}),
                      "No new macro guard API was introduced in structural cache paths");

    return result.processResult();
}
