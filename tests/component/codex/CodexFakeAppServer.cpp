/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

namespace {
    using Json = nlohmann::json;

    bool writeAll(int fd, std::string_view data) {
        std::size_t offset = 0;
        while (offset < data.size()) {
            const ssize_t written = ::write(fd, data.data() + offset, data.size() - offset);
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

    class LineReader {
    public:
        bool readLine(std::string& line, bool slow = false) {
            line.clear();
            char chunk[1024]{};

            while (true) {
                const std::size_t newline = buffer.find('\n');
                if (newline != std::string::npos) {
                    line.assign(buffer, 0, newline);
                    buffer.erase(0, newline + 1);
                    return true;
                }

                const ssize_t count = ::read(STDIN_FILENO, chunk, sizeof(chunk));
                if (count > 0) {
                    buffer.append(chunk, static_cast<std::size_t>(count));
                    if (slow) {
                        timespec delay{0, 1000000};
                        while (::nanosleep(&delay, &delay) != 0 && errno == EINTR) {
                        }
                    }
                } else if (count == 0) {
                    return false;
                } else if (errno != EINTR) {
                    return false;
                }
            }
        }

        bool waitForEof(bool rejectAdditionalData = false) {
            if (rejectAdditionalData && !buffer.empty()) {
                return false;
            }
            buffer.clear();

            char chunk[1024]{};
            while (true) {
                const ssize_t count = ::read(STDIN_FILENO, chunk, sizeof(chunk));
                if (count == 0) {
                    return true;
                }
                if (count > 0) {
                    if (rejectAdditionalData) {
                        return false;
                    }
                } else if (errno != EINTR) {
                    return false;
                }
            }
        }

    private:
        std::string buffer;
    };

    bool isBlockingDescriptor(int fd) {
        const int flags = ::fcntl(fd, F_GETFL);
        return flags >= 0 && (flags & O_NONBLOCK) == 0;
    }

    long long requestId(const std::string& request) {
        const std::size_t idField = request.find("\"id\"");
        if (idField == std::string::npos) {
            return 0;
        }
        const std::size_t colon = request.find(':', idField);
        if (colon == std::string::npos) {
            return 0;
        }
        return std::strtoll(request.c_str() + static_cast<std::ptrdiff_t>(colon + 1), nullptr, 10);
    }

    std::string initializeResponse(long long id) {
        return "{\"id\":" + std::to_string(id) +
               ",\"result\":{\"codexHome\":\"/tmp/fake-codex\",\"platformFamily\":\"unix\",\"platformOs\":\"linux\","
               "\"userAgent\":\"snodec-fake\"}}\n";
    }

    bool sendNormalHandshakeResponse(long long id) {
        const std::string response = initializeResponse(id);
        const std::size_t split = response.size() / 3;
        const std::string firstBatch = "{\"method\":\"fake/beforeInitialize\",\"params\":{}}\n"
                                       "{\"id\":999,\"result\":{\"ignored\":true}}\n" +
                                       response.substr(0, split);
        const std::string secondBatch = response.substr(split) + "{\"method\":\"fake/afterInitialize\",\"params\":{}}\n"
                                                                 "{\"method\":\"fake/secondNotification\",\"params\":{}}\n";

        return writeAll(STDOUT_FILENO, firstBatch) && writeAll(STDOUT_FILENO, secondBatch);
    }

    bool receiveInitialized(LineReader& input) {
        std::string initialized;
        return input.readLine(initialized) && !initialized.empty() && initialized.find("\"method\":\"initialized\"") != std::string::npos;
    }

    bool readJson(LineReader& input, Json& message) {
        std::string line;
        if (!input.readLine(line)) {
            return false;
        }

        try {
            message = Json::parse(line);
            return message.is_object();
        } catch (const Json::exception&) {
            return false;
        }
    }

    bool writeJson(const Json& message) {
        return writeAll(STDOUT_FILENO, message.dump() + '\n');
    }

    std::optional<long long> integerRequestId(const Json& request) {
        const auto id = request.find("id");
        if (id == request.end() || !id->is_number_integer()) {
            return std::nullopt;
        }

        try {
            return id->get<long long>();
        } catch (const Json::exception&) {
            return std::nullopt;
        }
    }

    Json fakeThread() {
        return {{"id", "thread-fake-001"},
                {"sessionId", "session-fake-001"},
                {"source", "appServer"},
                {"modelProvider", "openai"},
                {"createdAt", 1},
                {"updatedAt", 1},
                {"status", {{"type", "idle"}}},
                {"cwd", "/tmp/project"},
                {"cliVersion", "0.144.6"},
                {"preview", "Analyse the current branch."},
                {"ephemeral", true},
                {"turns", Json::array()}};
    }

    Json fakeTurn(std::string status) {
        return {{"id", "turn-fake-001"}, {"items", Json::array()}, {"status", std::move(status)}, {"startedAt", 2}};
    }

    Json approvalParams(std::string itemId) {
        return {{"threadId", "thread-fake-001"},
                {"turnId", "turn-fake-001"},
                {"itemId", std::move(itemId)},
                {"startedAtMs", 2000},
                {"command", "printf protocol-flow"},
                {"cwd", "/tmp/project"}};
    }

    Json threadOperationResult(const Json& thread) {
        return {{"approvalPolicy", "on-request"},
                {"approvalsReviewer", "user"},
                {"cwd", "/tmp/project"},
                {"model", "gpt-5"},
                {"modelProvider", "openai"},
                {"sandbox", {{"type", "readOnly"}}},
                {"thread", thread}};
    }

    bool receiveTypedRequest(LineReader& input, std::string_view method, const Json& params, long long& id) {
        Json request;
        const std::optional<long long> requestIdentifier = readJson(input, request) ? integerRequestId(request) : std::nullopt;
        if (!requestIdentifier || request.value("method", "") != method || request.value("params", Json()) != params) {
            return false;
        }
        id = *requestIdentifier;
        return true;
    }

    int runTypedFlow(LineReader& input) {
        const Json thread = fakeThread();
        long long id = 0;
        if (!receiveTypedRequest(input, "thread/start", Json{{"cwd", "/tmp/project"}}, id) ||
            !writeJson({{"id", id}, {"result", threadOperationResult(thread)}}) ||
            !writeJson({{"method", "thread/started"}, {"params", {{"thread", thread}}}})) {
            return 70;
        }

        if (!receiveTypedRequest(input, "thread/resume", Json{{"threadId", "thread-fake-001"}, {"cwd", "/tmp/project"}}, id) ||
            !writeJson({{"id", id}, {"result", threadOperationResult(thread)}})) {
            return 71;
        }

        if (!receiveTypedRequest(input, "thread/list", Json{{"cursor", "cursor-1"}, {"limit", 2}}, id) ||
            !writeJson(
                {{"id", id}, {"result", {{"data", Json::array({thread})}, {"nextCursor", "cursor-2"}, {"backwardsCursor", nullptr}}}})) {
            return 72;
        }

        if (!receiveTypedRequest(input, "thread/read", Json{{"threadId", "thread-fake-001"}, {"includeTurns", true}}, id) ||
            !writeJson({{"id", id}, {"result", {{"thread", thread}}}})) {
            return 73;
        }

        const Json turnStartParams = {
            {"threadId", "thread-fake-001"},
            {"input", Json::array({Json{{"type", "text"}, {"text", "Analyse the current branch."}, {"text_elements", Json::array()}}})},
            {"effort", "high"},
        };
        if (!receiveTypedRequest(input, "turn/start", turnStartParams, id)) {
            return 74;
        }

        const Json startedTurn = fakeTurn("inProgress");
        const Json agentItem = {{"type", "agentMessage"}, {"id", "message-fake-001"}, {"text", ""}};
        const Json commandItem = {{"type", "commandExecution"},
                                  {"id", "command-fake-001"},
                                  {"command", "printf typed-flow"},
                                  {"cwd", "/tmp/project"},
                                  {"status", "inProgress"},
                                  {"commandActions", Json::array()}};
        if (!writeJson({{"id", id}, {"result", {{"turn", startedTurn}}}}) ||
            !writeJson({{"method", "turn/started"}, {"params", {{"threadId", "thread-fake-001"}, {"turn", startedTurn}}}}) ||
            !writeJson(
                {{"method", "item/started"},
                 {"params", {{"threadId", "thread-fake-001"}, {"turnId", "turn-fake-001"}, {"item", agentItem}, {"startedAtMs", 2000}}}}) ||
            !writeJson({{"method", "item/agentMessage/delta"},
                        {"params",
                         {{"threadId", "thread-fake-001"},
                          {"turnId", "turn-fake-001"},
                          {"itemId", "message-fake-001"},
                          {"delta", "Analysis complete."}}}}) ||
            !writeJson({{"method", "item/started"},
                        {"params",
                         {{"threadId", "thread-fake-001"}, {"turnId", "turn-fake-001"}, {"item", commandItem}, {"startedAtMs", 2100}}}}) ||
            !writeJson(
                {{"method", "item/commandExecution/requestApproval"}, {"id", 800}, {"params", approvalParams("command-fake-001")}})) {
            return 75;
        }

        if (!receiveTypedRequest(input, "turn/interrupt", Json{{"threadId", "thread-fake-001"}, {"turnId", "turn-fake-001"}}, id) ||
            !writeJson({{"id", id}, {"result", Json::object()}})) {
            return 76;
        }

        Json response;
        if (!readJson(input, response) || response != Json{{"id", 800}, {"result", {{"decision", "accept"}}}}) {
            return 77;
        }

        const Json fileApproval = {{"threadId", "thread-fake-001"},
                                   {"turnId", "turn-fake-001"},
                                   {"itemId", "file-fake-001"},
                                   {"startedAtMs", 2200},
                                   {"reason", "apply deterministic patch"}};
        if (!writeJson({{"method", "item/fileChange/requestApproval"}, {"id", "file-801"}, {"params", fileApproval}}) ||
            !readJson(input, response) || response != Json{{"id", "file-801"}, {"result", {{"decision", "decline"}}}}) {
            return 78;
        }

        const Json userInput = {
            {"threadId", "thread-fake-001"},
            {"turnId", "turn-fake-001"},
            {"itemId", "input-fake-001"},
            {"questions",
             Json::array({Json{{"id", "scope"},
                               {"header", "Scope"},
                               {"question", "Which scope?"},
                               {"isOther", true},
                               {"options", Json::array({Json{{"label", "Current"}, {"description", "Current branch"}}})}}})},
        };
        if (!writeJson({{"method", "item/tool/requestUserInput"}, {"id", 802}, {"params", userInput}}) || !readJson(input, response) ||
            response != Json{{"id", 802}, {"result", {{"answers", {{"scope", {{"answers", Json::array({"Current"})}}}}}}}}) {
            return 79;
        }

        if (!writeJson({{"method", "future/serverRequest"}, {"id", "future-803"}, {"params", {{"future", true}}}}) ||
            !readJson(input, response) || response != Json{{"id", "future-803"}, {"result", {{"handled", true}}}}) {
            return 80;
        }

        const Json completedCommand = {{"type", "commandExecution"},
                                       {"id", "command-fake-001"},
                                       {"command", "printf typed-flow"},
                                       {"cwd", "/tmp/project"},
                                       {"status", "completed"},
                                       {"commandActions", Json::array()},
                                       {"aggregatedOutput", "typed-flow"},
                                       {"exitCode", 0}};
        const Json completedTurn = fakeTurn("completed");
        if (!writeJson({{"method", "future/event"}, {"params", {{"future", true}}}}) ||
            !writeJson({{"method", "item/started"},
                        {"params",
                         {{"threadId", "thread-fake-001"},
                          {"turnId", "turn-fake-001"},
                          {"item", {{"type", "futureItem"}, {"id", "future-item-001"}, {"future", true}}},
                          {"startedAtMs", 2300}}}}) ||
            !writeJson({{"method", "item/commandExecution/outputDelta"},
                        {"params",
                         {{"threadId", "thread-fake-001"},
                          {"turnId", "turn-fake-001"},
                          {"itemId", "command-fake-001"},
                          {"delta", "typed-flow"}}}}) ||
            !writeJson(
                {{"method", "item/completed"},
                 {"params",
                  {{"threadId", "thread-fake-001"}, {"turnId", "turn-fake-001"}, {"item", completedCommand}, {"completedAtMs", 2400}}}}) ||
            !writeJson({{"method", "turn/completed"}, {"params", {{"threadId", "thread-fake-001"}, {"turn", completedTurn}}}})) {
            return 81;
        }

        return input.waitForEof() ? 0 : 82;
    }

    int runProtocolFlow(LineReader& input) {
        constexpr std::size_t requestCount = 8;
        std::map<std::string, long long> ids;

        for (std::size_t requestIndex = 0; requestIndex < requestCount; ++requestIndex) {
            Json request;
            if (!readJson(input, request)) {
                return 40;
            }
            const auto method = request.find("method");
            const std::optional<long long> id = integerRequestId(request);
            if (method == request.end() || !method->is_string() || !id || !request.contains("params")) {
                return 41;
            }
            const auto [iterator, inserted] = ids.emplace(method->get<std::string>(), *id);
            if (!inserted || iterator->second != *id) {
                return 42;
            }

            if (*method == "thread/start" && request["params"] != Json{{"cwd", "/tmp/project"}}) {
                return 43;
            }
        }

        const std::vector<std::string> requiredMethods = {"thread/start",
                                                          "test/result-object",
                                                          "test/result-array",
                                                          "test/result-scalar",
                                                          "test/result-null",
                                                          "test/remote-error",
                                                          "test/out-of-order/first",
                                                          "test/out-of-order/second"};
        for (const std::string& method : requiredMethods) {
            if (!ids.contains(method)) {
                return 44;
            }
        }

        if (!writeJson({{"id", ids["test/out-of-order/second"]}, {"result", {{"order", "second"}}}}) ||
            !writeJson({{"id", ids["test/out-of-order/first"]}, {"result", {{"order", "first"}}}}) ||
            !writeJson({{"id", ids["test/remote-error"]},
                        {"error", {{"code", -32000}, {"message", "fake remote failure"}, {"data", {{"reason", "example"}}}}}}) ||
            !writeJson({{"id", ids["test/result-null"]}, {"result", nullptr}}) ||
            !writeJson({{"id", ids["test/result-scalar"]}, {"result", 17}}) ||
            !writeJson({{"id", ids["test/result-array"]}, {"result", Json::array({1, "two", false})}}) ||
            !writeJson({{"id", ids["test/result-object"]}, {"result", {{"shape", "object"}}}}) ||
            !writeJson({{"id", 987654}, {"result", {{"unmatched", true}}}}) ||
            !writeJson({{"id", "future-response-id"}, {"result", "unmatched-string"}}) ||
            !writeJson({{"method", "future/notification"}, {"params", Json::array({"preserved", 23})}})) {
            return 45;
        }

        const Json thread = fakeThread();
        if (!writeJson({{"method", "thread/started"}, {"params", {{"thread", thread}}}}) ||
            !writeJson({{"id", ids["thread/start"]},
                        {"result",
                         {{"approvalPolicy", "on-request"},
                          {"approvalsReviewer", "user"},
                          {"cwd", "/tmp/project"},
                          {"model", "gpt-5"},
                          {"modelProvider", "openai"},
                          {"sandbox", {{"type", "readOnly"}}},
                          {"thread", thread}}}})) {
            return 46;
        }

        Json turnStart;
        if (!readJson(input, turnStart) || turnStart.value("method", "") != "turn/start" || !integerRequestId(turnStart) ||
            turnStart.value("params", Json()) !=
                Json{{"threadId", "thread-fake-001"},
                     {"input", Json::array({Json{{"type", "text"}, {"text", "Analyse the current branch."}}})}}) {
            return 47;
        }

        const long long turnStartId = *integerRequestId(turnStart);
        const Json startedTurn = fakeTurn("inProgress");
        if (!writeJson({{"id", turnStartId}, {"result", {{"turn", startedTurn}}}}) ||
            !writeJson({{"method", "turn/started"}, {"params", {{"threadId", "thread-fake-001"}, {"turn", startedTurn}}}}) ||
            !writeJson({{"method", "item/agentMessage/delta"},
                        {"params",
                         {{"threadId", "thread-fake-001"},
                          {"turnId", "turn-fake-001"},
                          {"itemId", "message-fake-001"},
                          {"delta", "Analysis "}}}}) ||
            !writeJson({{"method", "item/commandExecution/requestApproval"}, {"id", 700}, {"params", approvalParams("command-700")}}) ||
            !writeJson({{"method", "item/commandExecution/requestApproval"},
                        {"id", "approval-string-701"},
                        {"params", approvalParams("command-701")}})) {
            return 48;
        }

        bool receivedIntegerResponse = false;
        bool receivedStringRejection = false;
        for (int responseIndex = 0; responseIndex < 2; ++responseIndex) {
            Json response;
            if (!readJson(input, response)) {
                return 49;
            }

            if (response.value("id", Json()) == Json(700)) {
                receivedIntegerResponse = response == Json{{"id", 700}, {"result", {{"decision", "accept"}}}};
            } else if (response.value("id", Json()) == Json("approval-string-701")) {
                receivedStringRejection =
                    response ==
                    Json{{"id", "approval-string-701"},
                         {"error", {{"code", -32000}, {"message", "Request rejected"}, {"data", {{"reason", "component-test"}}}}}};
            }
        }
        if (!receivedIntegerResponse || !receivedStringRejection) {
            return 50;
        }

        const Json completedTurn = fakeTurn("completed");
        if (!writeJson({{"method", "item/agentMessage/delta"},
                        {"params",
                         {{"threadId", "thread-fake-001"},
                          {"turnId", "turn-fake-001"},
                          {"itemId", "message-fake-001"},
                          {"delta", "complete."}}}}) ||
            !writeJson({{"method", "turn/completed"}, {"params", {{"threadId", "thread-fake-001"}, {"turn", completedTurn}}}})) {
            return 51;
        }

        return input.waitForEof() ? 0 : 52;
    }

    std::optional<long long> readStoredRequestId(const std::string& path) {
        const int marker = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (marker < 0) {
            return std::nullopt;
        }

        char value[64]{};
        ssize_t count = -1;
        do {
            count = ::read(marker, value, sizeof(value) - 1);
        } while (count < 0 && errno == EINTR);
        const bool closed = ::close(marker) == 0;
        if (count <= 0 || !closed) {
            return std::nullopt;
        }
        char* end = nullptr;
        const long long id = std::strtoll(value, &end, 10);
        return end != value && *end == '\0' ? std::optional<long long>(id) : std::nullopt;
    }

    int runProtocolExitOnce(LineReader& input, const std::string& markerPath, int firstLaunchMarker) {
        if (firstLaunchMarker >= 0) {
            Json pending;
            const std::optional<long long> id = readJson(input, pending) ? integerRequestId(pending) : std::nullopt;
            if (!id || pending.value("method", "") != "test/pending-after-exit") {
                ::close(firstLaunchMarker);
                return 60;
            }
            const std::string storedId = std::to_string(*id);
            if (!writeAll(firstLaunchMarker, storedId) || ::close(firstLaunchMarker) != 0) {
                return 61;
            }
            return 42;
        }

        const std::optional<long long> staleId = readStoredRequestId(markerPath);
        if (!staleId || !writeJson({{"id", *staleId}, {"result", {{"generation", "stale"}}}})) {
            return 62;
        }

        Json current;
        const std::optional<long long> currentId = readJson(input, current) ? integerRequestId(current) : std::nullopt;
        if (!currentId || current.value("method", "") != "test/pending-after-exit" || *currentId <= *staleId ||
            !writeJson({{"id", *currentId}, {"result", {{"generation", "current"}}}})) {
            return 63;
        }
        return input.waitForEof() ? 0 : 64;
    }
} // namespace

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "normal";
    int protocolExitMarker = -1;

    if (mode.starts_with("pidfile-")) {
        if (argc < 3) {
            return 30;
        }
        const int pidFile = ::open(argv[2], O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        if (pidFile < 0 || !writeAll(pidFile, std::to_string(::getpid())) || ::close(pidFile) != 0) {
            return 31;
        }
        mode.erase(0, std::string("pidfile-").size());
    }

    if (mode == "exit-before-initialize") {
        return 17;
    }

    if (mode == "protocol-exit-once") {
        if (argc < 3) {
            return 32;
        }
        protocolExitMarker = ::open(argv[2], O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (protocolExitMarker < 0 && errno != EEXIST) {
            return 33;
        }
    }

    if (!isBlockingDescriptor(STDIN_FILENO) || !isBlockingDescriptor(STDOUT_FILENO) || !isBlockingDescriptor(STDERR_FILENO)) {
        return 18;
    }

    if (mode == "never-read-stdin" || mode == "never-read-stdin-ignore-term") {
        if (mode == "never-read-stdin-ignore-term") {
            std::signal(SIGTERM, SIG_IGN);
        }
        while (true) {
            ::pause();
        }
    }

    LineReader input;
    std::string initialize;
    if (!input.readLine(initialize, mode == "slow-stdin") || initialize.empty() ||
        initialize.find("\"method\":\"initialize\"") == std::string::npos) {
        return 20;
    }

    if (mode == "close-stdout") {
        ::close(STDOUT_FILENO);
        static_cast<void>(input.waitForEof());
        return 0;
    }

    if (mode == "hold-initialize") {
        static_cast<void>(input.waitForEof());
        return 0;
    }

    bool rejectInitialization = mode == "initialization-error";
    if (mode == "fail-once") {
        if (argc < 3) {
            return 24;
        }
        const int marker = ::open(argv[2], O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (marker >= 0) {
            ::close(marker);
            rejectInitialization = true;
        } else if (errno != EEXIST) {
            return 25;
        }
    }

    if (rejectInitialization) {
        const std::string response = "{\"id\":" + std::to_string(requestId(initialize)) +
                                     ",\"error\":{\"code\":-32000,\"message\":\"fake initialization rejected\"}}\n";
        writeAll(STDOUT_FILENO, response);
        static_cast<void>(input.waitForEof());
        return 0;
    }

    if (mode == "malformed-json") {
        writeAll(STDOUT_FILENO, "{not-json}\n");
        static_cast<void>(input.waitForEof());
        return 0;
    }

    if (mode == "protocol-flow" || mode == "protocol-exit-once" || mode == "typed-flow") {
        if (!writeAll(STDERR_FILENO, "protocol-initialized\n") || !writeAll(STDOUT_FILENO, initializeResponse(requestId(initialize))) ||
            !receiveInitialized(input)) {
            if (protocolExitMarker >= 0) {
                ::close(protocolExitMarker);
            }
            return 34;
        }
        if (mode == "protocol-flow") {
            return runProtocolFlow(input);
        }
        if (mode == "typed-flow") {
            return runTypedFlow(input);
        }
        return runProtocolExitOnce(input, argv[2], protocolExitMarker);
    }

    if (!writeAll(STDERR_FILENO, "diagnostic one\ndiagnostic") || !writeAll(STDERR_FILENO, " two\n") ||
        !writeAll(STDERR_FILENO, "{\"id\":0,\"error\":{\"code\":99,\"message\":\"stderr only\"}}\n") ||
        !sendNormalHandshakeResponse(requestId(initialize)) || !receiveInitialized(input)) {
        return 21;
    }

    if (mode == "close-stderr") {
        if (!writeAll(STDERR_FILENO, "trailing diagnostic without newline")) {
            return 22;
        }
        ::close(STDERR_FILENO);
        if (!writeAll(STDOUT_FILENO, "{\"method\":\"fake/afterStderrClose\",\"params\":{}}\n")) {
            return 23;
        }
        return input.waitForEof() ? 0 : 26;
    }

    if (mode == "stdio-framing") {
        if (!input.waitForEof(true)) {
            return 27;
        }
        return writeAll(STDERR_FILENO, "stdio-framing-ok\n") ? 0 : 28;
    }

    if (!writeAll(STDERR_FILENO, mode == "slow-stdin" ? "slow-initialized\n" : "initialized-ok\n")) {
        return 22;
    }

    if (mode == "close-stdout-ignore-shutdown") {
        std::signal(SIGTERM, SIG_IGN);
        ::close(STDOUT_FILENO);
        static_cast<void>(input.waitForEof());
        while (true) {
            ::pause();
        }
    }

    if (mode == "ignore-shutdown") {
        std::signal(SIGTERM, SIG_IGN);
        static_cast<void>(input.waitForEof());
        while (true) {
            ::pause();
        }
    }

    return input.waitForEof() ? 0 : 29;
}
