#include "tests/policy/SourcePolicyTestRoot.h"

#include <cctype>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace {
    bool isSourceFile(const std::filesystem::path& path) {
        const std::string extension = path.extension().string();
        return extension == ".cpp" || extension == ".h" || extension == ".hpp";
    }

    std::vector<std::string> loggingStatements(const std::string& source) {
        constexpr std::string_view markers[] = {
            "LOG(", "PLOG(", ".trace(", ".debug(", ".info(", ".warn(", ".error(", ".critical(", ".sysError(",
            ".trace()", ".debug()", ".info()", ".warn()", ".error()", ".critical()",
        };
        std::vector<std::string> statements;
        std::size_t begin = 0;
        while (begin < source.size()) {
            std::size_t logPosition = std::string::npos;
            for (const std::string_view marker : markers) {
                const std::size_t candidate = source.find(marker, begin);
                if (candidate != std::string::npos && (logPosition == std::string::npos || candidate < logPosition)) {
                    logPosition = candidate;
                }
            }
            if (logPosition == std::string::npos) {
                break;
            }
            const std::size_t end = source.find(';', logPosition);
            if (end == std::string::npos) {
                break;
            }
            statements.push_back(source.substr(logPosition, end - logPosition + 1));
            begin = end + 1;
        }
        return statements;
    }

    void maskSafeMetadataAccess(std::string& statement, const std::string_view identifier, const bool getter) {
        for (std::size_t begin = statement.find(identifier); begin != std::string::npos;
             begin = statement.find(identifier, begin + 1)) {
            std::size_t cursor = begin + identifier.size();
            const auto skipWhitespace = [&statement, &cursor]() {
                while (cursor < statement.size() &&
                       std::isspace(static_cast<unsigned char>(statement[cursor])) != 0) {
                    ++cursor;
                }
            };
            skipWhitespace();
            if (getter) {
                if (cursor >= statement.size() || statement[cursor++] != '(') {
                    continue;
                }
                skipWhitespace();
                if (cursor >= statement.size() || statement[cursor++] != ')') {
                    continue;
                }
                skipWhitespace();
            }
            if (cursor >= statement.size() || statement[cursor++] != '.') {
                continue;
            }
            skipWhitespace();
            if (statement.compare(cursor, 5, "empty") == 0) {
                cursor += 5;
            } else if (statement.compare(cursor, 4, "size") == 0) {
                cursor += 4;
            } else {
                continue;
            }
            skipWhitespace();
            if (cursor >= statement.size() || statement[cursor++] != '(') {
                continue;
            }
            skipWhitespace();
            if (cursor >= statement.size() || statement[cursor++] != ')') {
                continue;
            }
            statement.replace(begin, cursor - begin, "SAFE_METADATA");
        }
    }

    std::string removeSafeMetadataUses(std::string statement) {
        for (const std::string_view identifier :
             {"getPassword", "getUsername", "getWillMessage", "queryAccessToken", "queryRefreshToken"}) {
            maskSafeMetadataAccess(statement, identifier, identifier.starts_with("get"));
        }
        return statement;
    }

    std::string maskStringLiteralContents(std::string statement) {
        bool inString = false;
        bool escaped = false;
        for (char& character : statement) {
            if (!inString) {
                if (character == '"') {
                    inString = true;
                }
                continue;
            }
            if (escaped) {
                character = ' ';
                escaped = false;
            } else if (character == '\\') {
                character = ' ';
                escaped = true;
            } else if (character == '"') {
                inString = false;
            } else {
                character = ' ';
            }
        }
        return statement;
    }
} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }

    // This is deliberately a source policy, not a claim that SNode.C has a
    // runtime redaction layer. It rejects direct secret-value expressions in
    // logging statements while allowing boolean/length metadata.
    const std::vector<std::regex> forbiddenValueExpressions = {
        std::regex(R"(getPassword\s*\(\s*\))", std::regex::icase),
        std::regex(R"(getUsername\s*\(\s*\))", std::regex::icase),
        std::regex(R"(getWillMessage\s*\(\s*\))", std::regex::icase),
        std::regex(R"(\b(?:query)?AccessToken\b)", std::regex::icase),
        std::regex(R"(\b(?:query)?RefreshToken\b)", std::regex::icase),
        std::regex(R"(\b(?:queryPassword|dbPassword(?:Hash|Salt)?)\b)", std::regex::icase),
        std::regex(R"(\b(?:clientSecret|certKeyPassword|privateKey|credentials?)\b)", std::regex::icase),
        std::regex(R"(\bdetails\s*\.\s*(?:username|password)\b)", std::regex::icase),
        std::regex(R"(\bsslConfig\s*\.\s*password\b)", std::regex::icase),
    };
    const std::vector<std::string> forbiddenRawExpressions = {
        "req->query(\"code\")",
        "req->query(\"access_token\")",
        "req->query(\"refresh_token\")",
        "body[\"password\"]",
        "jsonBody[\"access_token\"]",
    };

    bool ok = true;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root / "src")) {
        if (!entry.is_regular_file() || !isSourceFile(entry.path())) {
            continue;
        }
        const std::string source = source_policy::readSourcePolicyFile(entry.path());
        for (const std::string& originalStatement : loggingStatements(source)) {
            const std::string statement = removeSafeMetadataUses(originalStatement);
            const std::string expressionStatement = maskStringLiteralContents(statement);
            for (const auto& expression : forbiddenValueExpressions) {
                if (std::regex_search(expressionStatement, expression)) {
                    std::cerr << "Sensitive value expression in logging statement: " << entry.path() << '\n'
                              << originalStatement << '\n';
                    ok = false;
                    break;
                }
            }
            for (const std::string& expression : forbiddenRawExpressions) {
                if (statement.find(expression) != std::string::npos) {
                    std::cerr << "Sensitive request value in logging statement: " << entry.path() << '\n'
                              << originalStatement << '\n';
                    ok = false;
                }
            }
        }
    }

    return ok ? 0 : 1;
}
