#include "ai/openai/codex/backend/BackendEvent.h"
#include "ai/openai/codex/backend/BackendState.h"
#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/typed/Events.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace typed = ai::openai::codex::typed;

    class LoggerStateGuard {
    public:
        explicit LoggerStateGuard(const std::string& logFile) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::logToFile(logFile);
            logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
            logger::LogManager::setFormat(logger::LogManager::Format::Json);
            logger::LogManager::freeze();
        }

        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::init();
            logger::LogManager::init();
            errno = 0;
        }
    };

    typed::Thread thread(const std::string& id) {
        typed::Thread value;
        value.id = typed::ThreadId{id};
        value.raw = ai::openai::codex::Json::object();
        return value;
    }

    typed::Turn turn(const std::string& id, typed::TurnStatus status) {
        typed::Turn value;
        value.id = typed::TurnId{id};
        value.threadId = typed::ThreadId{"thread-phase3"};
        value.status = std::move(status);
        value.raw = ai::openai::codex::Json::object();
        return value;
    }

    std::vector<nlohmann::json> records(const std::filesystem::path& path) {
        std::vector<nlohmann::json> result;
        std::ifstream input(path);
        for (std::string line; std::getline(input, line);) {
            if (!line.empty()) {
                result.push_back(nlohmann::json::parse(line));
            }
        }
        return result;
    }

    int countMessage(const std::vector<nlohmann::json>& records, const std::string& message) {
        int count = 0;
        for (const auto& record : records) {
            if (record.value("message", "") == message) {
                ++count;
            }
        }
        return count;
    }
} // namespace

int main() {
    tests::support::TestResult result;
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "snodec-phase3-codex-lifecycle.jsonl";
    std::error_code error;
    std::filesystem::remove(path, error);

    {
        LoggerStateGuard loggerGuard(path.string());
        backend::BackendState state;
        backend::Reducer reducer;

        const auto applyTyped = [&state, &reducer](const typed::Event& event) {
            for (const backend::BackendEvent& translated : reducer.translate(event)) {
                reducer.apply(state, translated);
            }
        };

        reducer.apply(state, backend::ThreadUpserted{thread("thread-hydrated"), backend::EntityLoad::Summary});
        reducer.apply(state, backend::TurnUpserted{turn("turn-hydrated", typed::TurnStatus::inProgress())});
        applyTyped(typed::Event{typed::ThreadStarted{thread("thread-phase3"), ai::openai::codex::Json::object()}});

        typed::Turn success = turn("turn-success", typed::TurnStatus::inProgress());
        applyTyped(typed::Event{typed::TurnStarted{success, ai::openai::codex::Json::object()}});
        success.status = typed::TurnStatus::completed();
        applyTyped(typed::Event{typed::TurnCompleted{success, ai::openai::codex::Json::object()}});
        applyTyped(typed::Event{typed::TurnCompleted{success, ai::openai::codex::Json::object()}});

        typed::Turn failed = turn("turn-failed", typed::TurnStatus::inProgress());
        applyTyped(typed::Event{typed::TurnStarted{failed, ai::openai::codex::Json::object()}});
        failed.status = typed::TurnStatus::failed();
        applyTyped(typed::Event{typed::TurnFailed{failed, {{"message", "failure"}}, ai::openai::codex::Json::object()}});
        failed.status = typed::TurnStatus::completed();
        applyTyped(typed::Event{typed::TurnCompleted{failed, ai::openai::codex::Json::object()}});

        typed::Turn cancelled = turn("turn-cancelled", typed::TurnStatus::inProgress());
        applyTyped(typed::Event{typed::TurnStarted{cancelled, ai::openai::codex::Json::object()}});
        cancelled.status = typed::TurnStatus{"cancelled"};
        applyTyped(typed::Event{typed::TurnCompleted{cancelled, ai::openai::codex::Json::object()}});
        cancelled.status = typed::TurnStatus::completed();
        applyTyped(typed::Event{typed::TurnCompleted{cancelled, ai::openai::codex::Json::object()}});
    }

    const std::vector<nlohmann::json> captured = records(path);
    result.expectTrue(countMessage(captured, "thread created: thread=thread-phase3") == 1, "thread creation is emitted exactly once");
    result.expectTrue(countMessage(captured, "thread created: thread=thread-hydrated") == 0,
                      "generic thread hydration does not fabricate creation");
    result.expectTrue(countMessage(captured, "turn started: thread=thread-phase3 turn=turn-hydrated") == 0,
                      "generic turn hydration does not fabricate a start");
    result.expectTrue(countMessage(captured, "turn started: thread=thread-phase3 turn=turn-success") == 1, "successful turn has one start");
    result.expectTrue(countMessage(captured, "turn completed: thread=thread-phase3 turn=turn-success") == 1,
                      "successful turn has one completion");
    result.expectTrue(countMessage(captured, "turn failed: thread=thread-phase3 turn=turn-failed") == 1,
                      "failed turn has one failure and no later completion");
    result.expectTrue(countMessage(captured, "turn completed: thread=thread-phase3 turn=turn-failed") == 0,
                      "failed turn cannot later complete");
    result.expectTrue(countMessage(captured, "turn cancelled: thread=thread-phase3 turn=turn-cancelled") == 1,
                      "cancelled turn has one cancellation");
    result.expectTrue(countMessage(captured, "turn completed: thread=thread-phase3 turn=turn-cancelled") == 0,
                      "cancelled turn cannot later complete");

    for (const auto& record : captured) {
        const std::string message = record.value("message", "");
        if (message.starts_with("turn ")) {
            result.expectTrue(record.value("level", "") == "debug", "turn lifecycle uses Debug");
            result.expectTrue(record.value("origin", "") == "framework", "turn lifecycle uses framework origin");
            result.expectTrue(record.value("boundary", "") == "connection", "turn lifecycle uses connection boundary");
            result.expectTrue(record.value("component", "") == "ai.openai.codex.backend", "turn lifecycle uses backend component");
        }
    }

    return result.processResult();
}
