#include "tests/policy/SourcePolicyTestRoot.h"

#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

    struct Token {
        std::string text;
    };

    enum class Access { Public, Protected, Private };

    struct Declaration {
        Access access;
        std::vector<std::string> tokens;
    };

    bool isIdentifierStart(char character) {
        const unsigned char value = static_cast<unsigned char>(character);

        return std::isalpha(value) != 0 || character == '_';
    }

    bool isIdentifierCharacter(char character) {
        const unsigned char value = static_cast<unsigned char>(character);

        return std::isalnum(value) != 0 || character == '_';
    }

    std::vector<Token> tokenize(std::string_view source) {
        std::vector<Token> tokens;

        for (std::size_t position = 0; position < source.size();) {
            if (std::isspace(static_cast<unsigned char>(source[position])) != 0) {
                ++position;
                continue;
            }

            if (source.substr(position).starts_with("//")) {
                const std::size_t end = source.find('\n', position + 2);

                position = end == std::string_view::npos ? source.size() : end;
                continue;
            }

            if (source.substr(position).starts_with("/*")) {
                const std::size_t end = source.find("*/", position + 2);

                position = end == std::string_view::npos ? source.size() : end + 2;
                continue;
            }

            if (source[position] == '"' || source[position] == '\'') {
                const char quote = source[position++];

                while (position < source.size()) {
                    if (source[position] == '\\') {
                        position += position + 1 < source.size() ? 2 : 1;
                    } else if (source[position++] == quote) {
                        break;
                    }
                }

                continue;
            }

            if (isIdentifierStart(source[position])) {
                const std::size_t begin = position++;

                while (position < source.size() && isIdentifierCharacter(source[position])) {
                    ++position;
                }

                tokens.push_back({std::string(source.substr(begin, position - begin))});
                continue;
            }

            if (source.substr(position).starts_with("::")) {
                tokens.push_back({"::"});
                position += 2;
                continue;
            }

            tokens.push_back({std::string(1, source[position++])});
        }

        return tokens;
    }

    std::vector<Declaration> classDeclarations(const std::vector<Token>& tokens, std::string_view className) {
        std::size_t classOpen = tokens.size();

        for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
            if (tokens[index].text != "class" || tokens[index + 1].text != className) {
                continue;
            }

            std::size_t openingBrace = index + 2;

            while (openingBrace < tokens.size() && tokens[openingBrace].text != "{" && tokens[openingBrace].text != ";") {
                ++openingBrace;
            }

            if (openingBrace == tokens.size() || tokens[openingBrace].text == ";") {
                continue;
            }

            if (classOpen != tokens.size()) {
                std::cerr << "Class " << className << " has multiple definitions\n";
                return {};
            }

            classOpen = openingBrace;
        }

        if (classOpen == tokens.size()) {
            std::cerr << "Class " << className << " definition not found\n";
            return {};
        }

        std::vector<Declaration> declarations;
        std::vector<std::string> statement;

        Access access = Access::Private;
        std::size_t depth = 1;

        for (std::size_t index = classOpen + 1; index < tokens.size() && depth > 0; ++index) {
            const std::string& token = tokens[index].text;

            if (depth == 1 && (token == "public" || token == "protected" || token == "private") && index + 1 < tokens.size() &&
                tokens[index + 1].text == ":") {
                access = token == "public" ? Access::Public : token == "protected" ? Access::Protected : Access::Private;
                statement.clear();
                ++index;
                continue;
            }

            statement.push_back(token);

            if (token == "{") {
                ++depth;
            } else if (token == "}") {
                --depth;

                if (depth == 0) {
                    break;
                }
            } else if (token == ";" && depth == 1) {
                declarations.push_back({access, statement});
                statement.clear();
            }
        }

        return declarations;
    }

    bool containsFunctionName(const Declaration& declaration, std::string_view name) {
        for (std::size_t index = 0; index + 1 < declaration.tokens.size(); ++index) {
            if (declaration.tokens[index] == name && declaration.tokens[index + 1] == "(") {
                return true;
            }
        }

        return false;
    }

    bool requirePrivateDeclaration(const std::vector<Declaration>& declarations,
                                   std::string_view name,
                                   const std::vector<std::string>& expectedTokens,
                                   const std::filesystem::path& path) {
        std::vector<const Declaration*> matches;

        for (const Declaration& declaration : declarations) {
            if (containsFunctionName(declaration, name)) {
                matches.push_back(&declaration);
            }
        }

        if (matches.size() != 1) {
            std::cerr << "Logging helper " << name << " in " << path << " must have exactly one unambiguous declaration; found "
                      << matches.size() << '\n';
            return false;
        }

        bool ok = true;

        if (matches.front()->access != Access::Private) {
            std::cerr << "Logging helper " << name << " must remain private in " << path << '\n';
            ok = false;
        }

        if (matches.front()->tokens != expectedTokens) {
            std::cerr << "Logging helper " << name << " no longer has its required declaration in " << path << '\n';
            ok = false;
        }

        return ok;
    }

    bool forbidIdentifiers(const std::vector<Token>& tokens,
                           const std::vector<std::string_view>& identifiers,
                           const std::filesystem::path& path) {
        bool ok = true;

        for (const std::string_view identifier : identifiers) {
            for (const Token& token : tokens) {
                if (token.text == identifier) {
                    std::cerr << "Forbidden logging API/model identifier " << identifier << " appears in " << path << '\n';
                    ok = false;
                    break;
                }
            }
        }

        return ok;
    }

} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();

    if (root.empty()) {
        return 1;
    }

    const std::filesystem::path mqttPath = root / "src/iot/mqtt/Mqtt.h";
    const std::filesystem::path mqttClientPath = root / "src/iot/mqtt/client/Mqtt.h";
    const std::filesystem::path mqttServerPath = root / "src/iot/mqtt/server/Mqtt.h";

    const std::filesystem::path serverResponsePath = root / "src/web/http/server/Response.h";
    const std::filesystem::path clientRequestPath = root / "src/web/http/client/Request.h";
    const std::filesystem::path eventSourcePath = root / "src/web/http/client/tools/EventSource.h";

    const std::filesystem::path backendEventPath = root / "src/ai/openai/codex/backend/BackendEvent.h";
    const std::filesystem::path backendStatePath = root / "src/ai/openai/codex/backend/BackendState.h";

    const std::string mqttSource = source_policy::readSourcePolicyFile(mqttPath);
    const std::string mqttClientSource = source_policy::readSourcePolicyFile(mqttClientPath);
    const std::string mqttServerSource = source_policy::readSourcePolicyFile(mqttServerPath);

    const std::string serverResponseSource = source_policy::readSourcePolicyFile(serverResponsePath);
    const std::string clientRequestSource = source_policy::readSourcePolicyFile(clientRequestPath);
    const std::string eventSourceSource = source_policy::readSourcePolicyFile(eventSourcePath);

    const std::string backendEventSource = source_policy::readSourcePolicyFile(backendEventPath);
    const std::string backendStateSource = source_policy::readSourcePolicyFile(backendStatePath);

    if (mqttSource.empty() || mqttClientSource.empty() || mqttServerSource.empty() || serverResponseSource.empty() ||
        clientRequestSource.empty() || eventSourceSource.empty() || backendEventSource.empty() || backendStateSource.empty()) {
        std::cerr << "Unable to read one or more logging API policy inputs\n";
        return 1;
    }

    const std::vector<Token> mqttTokens = tokenize(mqttSource);
    const std::vector<Token> mqttClientTokens = tokenize(mqttClientSource);
    const std::vector<Token> mqttServerTokens = tokenize(mqttServerSource);

    const std::vector<Declaration> mqttDeclarations = classDeclarations(mqttTokens, "Mqtt");
    const std::vector<Declaration> mqttServerDeclarations = classDeclarations(mqttServerTokens, "Mqtt");

    bool ok = true;

    ok &= requirePrivateDeclaration(mqttDeclarations,
                                    "log",
                                    {
                                        "const",
                                        "logger",
                                        "::",
                                        "BoundaryLogger",
                                        "&",
                                        "log",
                                        "(",
                                        ")",
                                        "const",
                                        ";",
                                    },
                                    mqttPath);

    ok &= requirePrivateDeclaration(mqttDeclarations,
                                    "sessionEstablished",
                                    {
                                        "void",
                                        "sessionEstablished",
                                        "(",
                                        "bool",
                                        "resumed",
                                        ")",
                                        ";",
                                    },
                                    mqttPath);

    ok &= requirePrivateDeclaration(mqttDeclarations,
                                    "sessionRejected",
                                    {
                                        "void",
                                        "sessionRejected",
                                        "(",
                                        "const",
                                        "std",
                                        "::",
                                        "string",
                                        "&",
                                        "rejectedClientId",
                                        "=",
                                        "{",
                                        "}",
                                        ")",
                                        ";",
                                    },
                                    mqttPath);

    ok &= forbidIdentifiers(mqttClientTokens, {"clientLog"}, mqttClientPath);

    ok &= forbidIdentifiers(mqttServerTokens, {"serverLog"}, mqttServerPath);

    ok &= requirePrivateDeclaration(mqttServerDeclarations,
                                    "releaseSession",
                                    {
                                        "void",
                                        "releaseSession",
                                        "(",
                                        "logger",
                                        "::",
                                        "BoundaryLogger",
                                        "log",
                                        ",",
                                        "std",
                                        "::",
                                        "string_view",
                                        "prefix",
                                        ")",
                                        ";",
                                    },
                                    mqttServerPath);

    const std::vector<std::string_view> forbiddenHttpLoggingTypes = {
        "BoundaryLogger",
        "LogScopeOwner",
    };

    ok &= forbidIdentifiers(tokenize(serverResponseSource), forbiddenHttpLoggingTypes, serverResponsePath);

    ok &= forbidIdentifiers(tokenize(clientRequestSource), forbiddenHttpLoggingTypes, clientRequestPath);

    ok &= forbidIdentifiers(tokenize(eventSourceSource), forbiddenHttpLoggingTypes, eventSourcePath);

    const std::vector<std::string_view> forbiddenBackendIdentifiers = {
        "lifecycleStart",
        "creationLogged",
        "lifecycleStarted",
        "lifecycleTerminalLogged",
    };

    ok &= forbidIdentifiers(tokenize(backendEventSource), forbiddenBackendIdentifiers, backendEventPath);

    ok &= forbidIdentifiers(tokenize(backendStateSource), forbiddenBackendIdentifiers, backendStatePath);

    return ok ? 0 : 1;
}
