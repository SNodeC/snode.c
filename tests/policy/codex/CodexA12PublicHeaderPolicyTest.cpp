#include "tests/policy/SourcePolicyTestRoot.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace {
    struct PublicHeader {
        std::string_view relativePath;
        std::string_view guard;
    };

    std::size_t occurrenceCount(const std::string_view source, const std::string_view needle) {
        std::size_t count = 0;
        for (std::size_t position = source.find(needle); position != std::string_view::npos;
             position = source.find(needle, position + needle.size())) {
            ++count;
        }
        return count;
    }
} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }

    constexpr std::array<PublicHeader, 3> publicHeaders = {{
        {"src/ai/openai/codex/typed/Accounts.h", "AI_OPENAI_CODEX_TYPED_ACCOUNTS_H"},
        {"src/ai/openai/codex/typed/Models.h", "AI_OPENAI_CODEX_TYPED_MODELS_H"},
        {"src/ai/openai/codex/typed/Configuration.h", "AI_OPENAI_CODEX_TYPED_CONFIGURATION_H"},
    }};

    bool valid = true;
    for (const PublicHeader& header : publicHeaders) {
        const std::filesystem::path path = root / header.relativePath;
        const std::string source = source_policy::readSourcePolicyFile(path);
        const std::string ifndef = "#ifndef " + std::string(header.guard);
        const std::string define = "#define " + std::string(header.guard);
        const std::string endif = "#endif // " + std::string(header.guard);
        const std::size_t ifndefPosition = source.find(ifndef);
        const std::size_t definePosition = source.find(define);
        const std::size_t endifPosition = source.rfind(endif);

        const bool conventionalGuard = !source.empty() && occurrenceCount(source, ifndef) == 1 && occurrenceCount(source, define) == 1 &&
                                       occurrenceCount(source, endif) == 1 && ifndefPosition < definePosition &&
                                       definePosition < endifPosition;
        const bool avoidsPragmaOnce = source.find("#pragma once") == std::string::npos;
        if (!conventionalGuard || !avoidsPragmaOnce) {
            std::cerr << header.relativePath
                      << ": installed A1.2 public headers require one conventional include guard and no #pragma once\n";
            valid = false;
        }
    }

    return valid ? 0 : 1;
}
