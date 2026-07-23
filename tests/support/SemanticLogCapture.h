#ifndef TESTS_SUPPORT_SEMANTICLOGCAPTURE_H
#define TESTS_SUPPORT_SEMANTICLOGCAPTURE_H

#include "core/SNodeC.h"
#include "log/Logger.h"

#include <array>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tests::support {

    class SemanticLogCapture {
    public:
        explicit SemanticLogCapture(std::string testName)
            : path(std::filesystem::temp_directory_path() / (std::move(testName) + ".jsonl")) {
            std::error_code error;
            std::filesystem::remove(path, error);
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::logToFile(path.string());
        }

        ~SemanticLogCapture() {
            logger::Logger::disableLogToFile();
            logger::Logger::init();
            logger::LogManager::init();
            errno = 0;
        }

        SemanticLogCapture(const SemanticLogCapture&) = delete;
        SemanticLogCapture& operator=(const SemanticLogCapture&) = delete;

        void initCore(const char* testName) {
            arguments = {testName, "--log-level=6", "--log-format=json", "--quiet"};
            argumentPointers = {arguments[0].data(), arguments[1].data(), arguments[2].data(), arguments[3].data(), nullptr};
            core::SNodeC::init(4, argumentPointers.data());
        }

        std::vector<nlohmann::json> finish() {
            logger::Logger::disableLogToFile();
            std::vector<nlohmann::json> records;
            std::ifstream input(path);
            for (std::string line; std::getline(input, line);) {
                if (!line.empty()) {
                    records.push_back(nlohmann::json::parse(line));
                }
            }
            return records;
        }

        static int count(const std::vector<nlohmann::json>& records,
                         std::string_view component,
                         std::string_view instance,
                         std::string_view messagePrefix) {
            int matches = 0;
            for (const nlohmann::json& record : records) {
                const std::string message = stringValue(record, "message");
                if (stringValue(record, "component") == component && stringValue(record, "instance") == instance &&
                    message.starts_with(messagePrefix)) {
                    ++matches;
                }
            }
            return matches;
        }

        static bool matchingIdentityAndLevel(const std::vector<nlohmann::json>& records,
                                             std::string_view component,
                                             std::string_view instance,
                                             std::string_view messagePrefix,
                                             std::string_view level,
                                             bool requireConnection = true,
                                             std::string_view role = {}) {
            for (const nlohmann::json& record : records) {
                const std::string message = stringValue(record, "message");
                if (stringValue(record, "component") == component && stringValue(record, "instance") == instance &&
                    message.starts_with(messagePrefix)) {
                    if (stringValue(record, "level") != level || stringValue(record, "origin") != "framework" ||
                        stringValue(record, "boundary") != "connection" ||
                        (requireConnection && stringValue(record, "connection").empty()) ||
                        (!role.empty() && stringValue(record, "role") != role)) {
                        return false;
                    }
                }
            }
            return true;
        }

        static std::size_t
        position(const std::vector<nlohmann::json>& records, std::string_view component, std::string_view messagePrefix) {
            for (std::size_t index = 0; index < records.size(); ++index) {
                const std::string message = stringValue(records[index], "message");
                if (stringValue(records[index], "component") == component && message.starts_with(messagePrefix)) {
                    return index;
                }
            }
            return records.size();
        }

    private:
        static std::string stringValue(const nlohmann::json& record, const char* key) {
            const auto value = record.find(key);
            return value != record.end() && value->is_string() ? value->get<std::string>() : std::string();
        }

        std::filesystem::path path;
        std::array<std::string, 4> arguments;
        std::array<char*, 5> argumentPointers{};
    };

} // namespace tests::support

#endif // TESTS_SUPPORT_SEMANTICLOGCAPTURE_H
