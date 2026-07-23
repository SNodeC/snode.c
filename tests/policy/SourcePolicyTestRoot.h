#ifndef TESTS_POLICY_SOURCEPOLICYTESTROOT_H
#define TESTS_POLICY_SOURCEPOLICYTESTROOT_H

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace source_policy {

    inline constexpr auto kProjectRootMarker = "src/SemanticLog.h";

    inline std::filesystem::path sourcePolicyProjectRoot() {
        if (const char* sourceDir = std::getenv("SNODEC_SOURCE_DIR")) {
            const std::filesystem::path root = sourceDir;
            if (std::filesystem::exists(root / kProjectRootMarker)) {
                return root;
            }

            std::cerr << "SNODEC_SOURCE_DIR does not look like the project root: " << root << '\n';
            return {};
        }

        std::filesystem::path current = std::filesystem::current_path();
        while (!current.empty()) {
            if (std::filesystem::exists(current / kProjectRootMarker)) {
                return current;
            }

            const std::filesystem::path parent = current.parent_path();
            if (parent == current) {
                break;
            }
            current = parent;
        }

        std::cerr << "Unable to locate SNode.C project root from " << std::filesystem::current_path() << '\n';
        return {};
    }

    inline std::string readSourcePolicyFile(const std::filesystem::path& path) {
        std::ifstream input(path);
        std::ostringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    }

    inline bool contains(std::string_view haystack, std::string_view needle) {
        return haystack.find(needle) != std::string_view::npos;
    }

} // namespace source_policy

#endif // TESTS_POLICY_SOURCEPOLICYTESTROOT_H
