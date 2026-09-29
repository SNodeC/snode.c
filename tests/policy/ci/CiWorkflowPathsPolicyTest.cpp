#include "tests/policy/SourcePolicyTestRoot.h"

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

    using Filters = std::map<std::string, std::set<std::string>>;
    using Triggers = std::map<std::string, Filters>;

    // Read trigger mappings with inline or block lists; ignore the workflow jobs.
    bool matchesTriggers(std::string_view source, const Triggers& expected) {
        Triggers events;
        std::istringstream lines{std::string(source)};
        std::string line;
        std::string event;
        std::string filter;
        bool inOn = false;
        bool foundOn = false;

        while (std::getline(lines, line)) {
            const std::size_t indent = line.find_first_not_of(' ');
            if (indent == std::string::npos || trim(line).empty() || line[indent] == '#') {
                continue;
            }
            const std::string content = trim(std::string_view(line).substr(indent));
            if (indent == 0) {
                inOn = content == "on:" || content == "'on':" || content == "\"on\":";
                if (inOn && foundOn) {
                    return false;
                }
                foundOn = foundOn || inOn;
                event.clear();
                filter.clear();
                continue;
            }
            if (!inOn) {
                continue;
            }
            if (indent == 2 && content.ends_with(':')) {
                event = unquote(content.substr(0, content.size() - 1));
                if (!events.emplace(event, Filters{}).second) {
                    return false;
                }
                filter.clear();
            } else if (indent == 4 && !event.empty()) {
                const auto colon = content.find(':');
                if (colon == std::string::npos) {
                    return false;
                }
                filter = unquote(trim(std::string_view(content).substr(0, colon)));
                if (!events[event].emplace(filter, std::set<std::string>{}).second) {
                    return false;
                }
                const std::string values = trim(std::string_view(content).substr(colon + 1));
                if (!values.empty()) {
                    if (values.front() != '[' || values.back() != ']') {
                        return false;
                    }
                    std::istringstream items(values.substr(1, values.size() - 2));
                    std::string item;
                    while (std::getline(items, item, ',')) {
                        events[event][filter].insert(unquote(trim(item)));
                    }
                }
            } else if (indent == 6 && !filter.empty() && content.starts_with("- ")) {
                events[event][filter].insert(unquote(trim(std::string_view(content).substr(2))));
            } else {
                return false;
            }
        }
        return foundOn && events == expected;
    }

} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }
    const std::map<std::string, Triggers> workflows = {
        {"main.yml", {{"push", {{"branches", {"master"}}, {"paths", {"README.md"}}}}}},
        {"openwrt.yml", {{"push", {{"tags", {"v[0-9]*.[0-9]*.[0-9]*"}}}}}},
    };
    bool ok = true;
    for (const auto& [name, triggers] : workflows) {
        const auto path = root / ".github/workflows" / name;
        if (!matchesTriggers(source_policy::readSourcePolicyFile(path), triggers)) {
            std::cerr << "Unexpected CI triggers in " << path << ": only README TOC updates and version-tag notifications are allowed\n";
            ok = false;
        }
    }
    for (const auto& entry : std::filesystem::directory_iterator(root / ".github/workflows")) {
        if ((entry.path().extension() == ".yml" || entry.path().extension() == ".yaml") &&
            !workflows.contains(entry.path().filename().string())) {
            std::cerr << "Unexpected additional CI workflow: " << entry.path() << '\n';
            ok = false;
        }
    }
    return ok ? 0 : 1;
}
