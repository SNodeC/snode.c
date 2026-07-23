#include "tests/policy/SourcePolicyTestRoot.h"

#include <array>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

namespace {

    std::string trim(std::string_view value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
            value.remove_prefix(1);
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
            value.remove_suffix(1);
        }

        return std::string(value);
    }

    std::string unquote(std::string value) {
        if (value.size() >= 2 && ((value.front() == '\'' && value.back() == '\'') || (value.front() == '"' && value.back() == '"'))) {
            return value.substr(1, value.size() - 2);
        }

        return value;
    }

    struct TriggerPaths {
        std::map<std::string, std::set<std::string>> byEvent;
        std::set<std::string> duplicateEvents;
        std::set<std::string> eventsWithPaths;
        std::set<std::string> duplicatePathMappings;
        bool foundOn = false;
        bool duplicateOn = false;
    };

    TriggerPaths parseTriggerPaths(std::string_view source) {
        TriggerPaths result;
        std::istringstream lines{std::string(source)};
        std::string line;
        std::string event;
        bool inOn = false;
        bool inPaths = false;

        while (std::getline(lines, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            const std::size_t indent = line.find_first_not_of(' ');
            if (indent == std::string::npos || line[indent] == '#') {
                continue;
            }
            if (line.find('\t', 0) != std::string::npos) {
                std::cerr << "CI workflow trigger policy requires space indentation\n";
                return {};
            }

            const std::string content = trim(std::string_view(line).substr(indent));
            if (indent == 0) {
                inOn = content == "on:";
                result.duplicateOn = result.duplicateOn || (inOn && result.foundOn);
                inPaths = false;
                event.clear();
                result.foundOn = result.foundOn || inOn;
                continue;
            }
            if (!inOn) {
                continue;
            }
            if (indent == 2 && content.ends_with(':')) {
                event = content.substr(0, content.size() - 1);
                if (result.byEvent.contains(event)) {
                    result.duplicateEvents.insert(event);
                } else {
                    result.byEvent.emplace(event, std::set<std::string>{});
                }
                inPaths = false;
                continue;
            }
            if (event.empty()) {
                continue;
            }
            if (indent == 4) {
                inPaths = content == "paths:";
                if (inPaths && !result.eventsWithPaths.insert(event).second) {
                    result.duplicatePathMappings.insert(event);
                }
                continue;
            }
            if (inPaths && indent == 6 && content.starts_with("- ")) {
                result.byEvent[event].insert(unquote(trim(std::string_view(content).substr(2))));
            } else if (indent <= 6) {
                inPaths = false;
            }
        }

        return result;
    }

} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }

    const std::filesystem::path workflowPath = root / ".github/workflows/ci.yml";
    const TriggerPaths triggerPaths = parseTriggerPaths(source_policy::readSourcePolicyFile(workflowPath));
    const std::array<std::string_view, 2> requiredEvents = {"push", "pull_request"};
    const std::array<std::string_view, 7> requiredPaths = {
        "CMakeLists.txt",
        "cmake/**",
        "src/**",
        "tests/**",
        "tools/**",
        "docs/ai/openai/codex/**",
        ".github/workflows/ci.yml",
    };

    bool ok = triggerPaths.foundOn;
    if (!triggerPaths.foundOn) {
        std::cerr << "CI workflow has no top-level on trigger mapping: " << workflowPath << '\n';
    }
    if (triggerPaths.duplicateOn) {
        std::cerr << "CI workflow has duplicate top-level on trigger mappings: " << workflowPath << '\n';
        ok = false;
    }

    for (const std::string_view event : requiredEvents) {
        const auto eventPaths = triggerPaths.byEvent.find(std::string(event));
        if (eventPaths == triggerPaths.byEvent.end()) {
            std::cerr << "CI workflow has no " << event << " trigger mapping\n";
            ok = false;
            continue;
        }
        if (triggerPaths.duplicateEvents.contains(std::string(event))) {
            std::cerr << "CI workflow has duplicate " << event << " trigger mappings\n";
            ok = false;
        }
        if (triggerPaths.duplicatePathMappings.contains(std::string(event))) {
            std::cerr << "CI workflow has duplicate " << event << " paths mappings\n";
            ok = false;
        }
        for (const std::string_view requiredPath : requiredPaths) {
            if (!eventPaths->second.contains(std::string(requiredPath))) {
                std::cerr << "CI workflow " << event << " paths omit required path family: " << requiredPath << '\n';
                ok = false;
            }
        }
    }

    return ok ? 0 : 1;
}
