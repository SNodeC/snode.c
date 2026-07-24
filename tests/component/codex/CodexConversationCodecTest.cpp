/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "support/TestResult.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
    using ai::openai::codex::Json;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    const std::filesystem::path FixtureRoot = CODEX_A1_FIXTURE_ROOT;

    Json fixture(const std::filesystem::path& relative) {
        std::ifstream input(FixtureRoot / relative);
        return Json::parse(input);
    }

    bool isForwardCompatible(const std::optional<typed::DecodeDiagnostic>& diagnostic,
                             typed::DecodeIssueKind kind = typed::DecodeIssueKind::UnknownDiscriminator) {
        return diagnostic && diagnostic->kind == kind && diagnostic->severity == typed::DecodeIssueSeverity::ForwardCompatibility;
    }

    bool isProtocolWarning(const std::optional<typed::DecodeDiagnostic>& diagnostic) {
        return diagnostic && diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
               diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning;
    }

    bool omitsSensitiveValue(const typed::DecodeDiagnostic& diagnostic, const std::string_view sensitive) {
        return diagnostic.surface.find(sensitive) == std::string::npos && diagnostic.fieldPath.find(sensitive) == std::string::npos &&
               diagnostic.message.find(sensitive) == std::string::npos;
    }

    template <typename T>
    bool
    nullableStringMatches(const typed::OptionalNullable<T>& value, const Json& object, const std::string_view field, const auto& convert) {
        const auto member = object.find(field);
        if (member == object.end()) {
            return value.isOmitted();
        }
        if (member->is_null()) {
            return value.isNull();
        }
        return value.hasValue() && convert(*value.value) == *member;
    }

    bool optionalBoolMatches(const std::optional<bool>& value, const Json& object, const std::string_view field) {
        const auto member = object.find(field);
        return member == object.end() ? !value : value && member->is_boolean() && *value == member->get<bool>();
    }

    struct Observation {
        bool hasValue = false;
        bool explicitUnknown = false;
        bool rawRetained = false;
        bool fieldsMapped = false;
        std::string label;
        std::optional<typed::DecodeDiagnostic> diagnostic;
    };

    Observation observeAskForApproval(const Json& raw) {
        const auto decoded = detail::decodeAskForApproval(raw);
        Observation result{.hasValue = decoded.value.has_value(), .diagnostic = decoded.diagnostic};
        if (!decoded.value) {
            return result;
        }
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, typed::ApprovalPolicy>) {
                    result.label = value.value;
                    result.rawRetained = raw.is_string() && raw == value.value;
                    result.fieldsMapped = result.rawRetained;
                } else if constexpr (std::is_same_v<Value, typed::GranularAskForApproval>) {
                    result.label = "granular";
                    result.rawRetained = value.raw == raw;
                    const Json& options = raw.at("granular");
                    result.fieldsMapped = value.granular.mcpElicitations == options.at("mcp_elicitations").get<bool>() &&
                                          optionalBoolMatches(value.granular.requestPermissions, options, "request_permissions") &&
                                          value.granular.rules == options.at("rules").get<bool>() &&
                                          value.granular.sandboxApproval == options.at("sandbox_approval").get<bool>() &&
                                          optionalBoolMatches(value.granular.skillApproval, options, "skill_approval");
                } else {
                    result.label = "future";
                    result.explicitUnknown = true;
                    result.rawRetained = value.raw == raw;
                    result.fieldsMapped = value.discriminator.has_value();
                }
            },
            *decoded.value);
        return result;
    }

    Observation observeCommandAction(const Json& raw) {
        const auto decoded = detail::decodeCommandAction(raw);
        Observation result{.hasValue = decoded.value.has_value(), .diagnostic = decoded.diagnostic};
        if (!decoded.value) {
            return result;
        }
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                result.rawRetained = value.raw == raw;
                if constexpr (std::is_same_v<Value, typed::ReadCommandAction>) {
                    result.label = "read";
                    result.fieldsMapped = value.command == raw.at("command").get<std::string>() &&
                                          value.name == raw.at("name").get<std::string>() &&
                                          value.path.value == raw.at("path").get<std::string>();
                } else if constexpr (std::is_same_v<Value, typed::ListFilesCommandAction>) {
                    result.label = "listFiles";
                    result.fieldsMapped = value.command == raw.at("command").get<std::string>() &&
                                          nullableStringMatches(value.path, raw, "path", [](const std::string& input) {
                                              return Json(input);
                                          });
                } else if constexpr (std::is_same_v<Value, typed::SearchCommandAction>) {
                    result.label = "search";
                    result.fieldsMapped = value.command == raw.at("command").get<std::string>() &&
                                          nullableStringMatches(value.path,
                                                                raw,
                                                                "path",
                                                                [](const std::string& input) {
                                                                    return Json(input);
                                                                }) &&
                                          nullableStringMatches(value.query, raw, "query", [](const std::string& input) {
                                              return Json(input);
                                          });
                } else if constexpr (std::is_same_v<Value, typed::UnknownCommandAction>) {
                    result.label = "unknown";
                    result.fieldsMapped = value.command == raw.at("command").get<std::string>();
                } else {
                    result.label = "future";
                    result.explicitUnknown = true;
                    result.fieldsMapped = value.type.has_value();
                }
            },
            *decoded.value);
        return result;
    }

    Observation observeDynamicOutput(const Json& raw) {
        const auto decoded = detail::decodeDynamicToolCallOutputContentItem(raw);
        Observation result{.hasValue = decoded.value.has_value(), .diagnostic = decoded.diagnostic};
        if (!decoded.value) {
            return result;
        }
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                result.rawRetained = value.raw == raw;
                if constexpr (std::is_same_v<Value, typed::InputTextDynamicToolCallOutputContentItem>) {
                    result.label = "inputText";
                    result.fieldsMapped = value.text == raw.at("text").get<std::string>();
                } else if constexpr (std::is_same_v<Value, typed::InputImageDynamicToolCallOutputContentItem>) {
                    result.label = "inputImage";
                    result.fieldsMapped = value.imageUrl == raw.at("imageUrl").get<std::string>();
                } else {
                    result.label = "future";
                    result.explicitUnknown = true;
                    result.fieldsMapped = value.type.has_value();
                }
            },
            *decoded.value);
        return result;
    }

    Observation observePatchChangeKind(const Json& raw) {
        const auto decoded = detail::decodePatchChangeKind(raw);
        Observation result{.hasValue = decoded.value.has_value(), .diagnostic = decoded.diagnostic};
        if (!decoded.value) {
            return result;
        }
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                result.rawRetained = value.raw == raw;
                if constexpr (std::is_same_v<Value, typed::AddPatchChangeKind>) {
                    result.label = "add";
                    result.fieldsMapped = true;
                } else if constexpr (std::is_same_v<Value, typed::DeletePatchChangeKind>) {
                    result.label = "delete";
                    result.fieldsMapped = true;
                } else if constexpr (std::is_same_v<Value, typed::UpdatePatchChangeKind>) {
                    result.label = "update";
                    result.fieldsMapped = nullableStringMatches(value.movePath, raw, "move_path", [](const std::string& input) {
                        return Json(input);
                    });
                } else {
                    result.label = "future";
                    result.explicitUnknown = true;
                    result.fieldsMapped = value.type.has_value();
                }
            },
            *decoded.value);
        return result;
    }

    Observation observeSandboxPolicy(const Json& raw) {
        const auto decoded = detail::decodeSandboxPolicy(raw);
        Observation result{.hasValue = decoded.value.has_value(), .diagnostic = decoded.diagnostic};
        if (!decoded.value) {
            return result;
        }
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                result.rawRetained = value.raw == raw;
                if constexpr (std::is_same_v<Value, typed::DangerFullAccessSandboxPolicy>) {
                    result.label = "dangerFullAccess";
                    result.fieldsMapped = true;
                } else if constexpr (std::is_same_v<Value, typed::ReadOnlySandboxPolicy>) {
                    result.label = "readOnly";
                    result.fieldsMapped = optionalBoolMatches(value.networkAccess, raw, "networkAccess");
                } else if constexpr (std::is_same_v<Value, typed::ExternalSandboxSandboxPolicy>) {
                    result.label = "externalSandbox";
                    const auto member = raw.find("networkAccess");
                    result.fieldsMapped = member == raw.end() ? !value.networkAccess
                                                              : value.networkAccess && member->is_string() &&
                                                                    value.networkAccess->value == member->get<std::string>();
                } else if constexpr (std::is_same_v<Value, typed::WorkspaceWriteSandboxPolicy>) {
                    result.label = "workspaceWrite";
                    const auto roots = raw.find("writableRoots");
                    bool rootsMatch = roots == raw.end()
                                          ? !value.writableRoots
                                          : value.writableRoots && roots->is_array() && value.writableRoots->size() == roots->size();
                    if (rootsMatch && roots != raw.end()) {
                        for (std::size_t index = 0; index < roots->size(); ++index) {
                            rootsMatch = rootsMatch && value.writableRoots->at(index).value == roots->at(index).get<std::string>();
                        }
                    }
                    result.fieldsMapped = rootsMatch && optionalBoolMatches(value.networkAccess, raw, "networkAccess") &&
                                          optionalBoolMatches(value.excludeTmpdirEnvVar, raw, "excludeTmpdirEnvVar") &&
                                          optionalBoolMatches(value.excludeSlashTmp, raw, "excludeSlashTmp");
                } else {
                    result.label = "future";
                    result.explicitUnknown = true;
                    result.fieldsMapped = value.type.has_value();
                }
            },
            *decoded.value);
        return result;
    }

    bool textElementsMatch(const std::optional<std::vector<typed::TextElement>>& values, const Json& raw) {
        const auto member = raw.find("text_elements");
        if (member == raw.end()) {
            return !values;
        }
        if (!values || !member->is_array() || values->size() != member->size()) {
            return false;
        }
        for (std::size_t index = 0; index < values->size(); ++index) {
            const auto& value = values->at(index);
            const Json& item = member->at(index);
            if (value.byteRange.start != item.at("byteRange").at("start").get<std::uint64_t>() ||
                value.byteRange.end != item.at("byteRange").at("end").get<std::uint64_t>() ||
                !nullableStringMatches(value.placeholder, item, "placeholder", [](const std::string& input) {
                    return Json(input);
                })) {
                return false;
            }
        }
        return true;
    }

    Observation observeUserInput(const Json& raw) {
        const auto decoded = detail::decodeUserInput(raw);
        Observation result{.hasValue = decoded.value.has_value(), .diagnostic = decoded.diagnostic};
        if (!decoded.value) {
            return result;
        }
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                result.rawRetained = value.raw == raw;
                if constexpr (std::is_same_v<Value, typed::TextUserInput>) {
                    result.label = "text";
                    result.fieldsMapped = value.text == raw.at("text").get<std::string>() && textElementsMatch(value.textElements, raw);
                } else if constexpr (std::is_same_v<Value, typed::ImageUserInput>) {
                    result.label = "image";
                    result.fieldsMapped = value.url == raw.at("url").get<std::string>() &&
                                          nullableStringMatches(value.detail, raw, "detail", [](const typed::ImageDetail& input) {
                                              return Json(input.value);
                                          });
                } else if constexpr (std::is_same_v<Value, typed::LocalImageUserInput>) {
                    result.label = "localImage";
                    result.fieldsMapped = value.path == raw.at("path").get<std::string>() &&
                                          nullableStringMatches(value.detail, raw, "detail", [](const typed::ImageDetail& input) {
                                              return Json(input.value);
                                          });
                } else if constexpr (std::is_same_v<Value, typed::SkillUserInput>) {
                    result.label = "skill";
                    result.fieldsMapped =
                        value.name == raw.at("name").get<std::string>() && value.path == raw.at("path").get<std::string>();
                } else if constexpr (std::is_same_v<Value, typed::MentionUserInput>) {
                    result.label = "mention";
                    result.fieldsMapped =
                        value.name == raw.at("name").get<std::string>() && value.path == raw.at("path").get<std::string>();
                } else {
                    result.label = "future";
                    result.explicitUnknown = true;
                    result.fieldsMapped = value.type.has_value();
                }
            },
            *decoded.value);
        return result;
    }

    Observation observeWebSearchAction(const Json& raw) {
        const auto decoded = detail::decodeWebSearchAction(raw);
        Observation result{.hasValue = decoded.value.has_value(), .diagnostic = decoded.diagnostic};
        if (!decoded.value) {
            return result;
        }
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                result.rawRetained = value.raw == raw;
                if constexpr (std::is_same_v<Value, typed::SearchWebSearchAction>) {
                    result.label = "search";
                    result.fieldsMapped = nullableStringMatches(value.queries,
                                                                raw,
                                                                "queries",
                                                                [](const std::vector<std::string>& input) {
                                                                    return Json(input);
                                                                }) &&
                                          nullableStringMatches(value.query, raw, "query", [](const std::string& input) {
                                              return Json(input);
                                          });
                } else if constexpr (std::is_same_v<Value, typed::OpenPageWebSearchAction>) {
                    result.label = "openPage";
                    result.fieldsMapped = nullableStringMatches(value.url, raw, "url", [](const std::string& input) {
                        return Json(input);
                    });
                } else if constexpr (std::is_same_v<Value, typed::FindInPageWebSearchAction>) {
                    result.label = "findInPage";
                    result.fieldsMapped = nullableStringMatches(value.pattern,
                                                                raw,
                                                                "pattern",
                                                                [](const std::string& input) {
                                                                    return Json(input);
                                                                }) &&
                                          nullableStringMatches(value.url, raw, "url", [](const std::string& input) {
                                              return Json(input);
                                          });
                } else if constexpr (std::is_same_v<Value, typed::OtherWebSearchAction>) {
                    result.label = "other";
                    result.fieldsMapped = true;
                } else {
                    result.label = "future";
                    result.explicitUnknown = true;
                    result.fieldsMapped = value.type.has_value();
                }
            },
            *decoded.value);
        return result;
    }

    Observation observe(std::string_view domain, const Json& raw) {
        if (domain == "AskForApproval") {
            return observeAskForApproval(raw);
        }
        if (domain == "CommandAction") {
            return observeCommandAction(raw);
        }
        if (domain == "DynamicToolCallOutputContentItem") {
            return observeDynamicOutput(raw);
        }
        if (domain == "PatchChangeKind") {
            return observePatchChangeKind(raw);
        }
        if (domain == "SandboxPolicy") {
            return observeSandboxPolicy(raw);
        }
        if (domain == "UserInput") {
            return observeUserInput(raw);
        }
        if (domain == "WebSearchAction") {
            return observeWebSearchAction(raw);
        }
        return {};
    }

    bool isB2Domain(const std::string_view domain) {
        constexpr std::array Domains{
            std::string_view{"AskForApproval"},
            std::string_view{"CommandAction"},
            std::string_view{"DynamicToolCallOutputContentItem"},
            std::string_view{"PatchChangeKind"},
            std::string_view{"SandboxPolicy"},
            std::string_view{"UserInput"},
            std::string_view{"WebSearchAction"},
        };
        for (const std::string_view expected : Domains) {
            if (domain == expected) {
                return true;
            }
        }
        return false;
    }

    std::optional<Json> encodeBidirectional(std::string_view domain, const Json& raw, std::string& error) {
        if (domain == "AskForApproval") {
            const auto decoded = detail::decodeAskForApproval(raw);
            return decoded.value ? detail::encodeAskForApproval(*decoded.value, error) : std::nullopt;
        }
        if (domain == "SandboxPolicy") {
            const auto decoded = detail::decodeSandboxPolicy(raw);
            return decoded.value ? detail::encodeSandboxPolicy(*decoded.value, error) : std::nullopt;
        }
        if (domain == "UserInput") {
            const auto decoded = detail::decodeUserInput(raw);
            return decoded.value ? detail::encodeUserInput(*decoded.value, error) : std::nullopt;
        }
        return std::nullopt;
    }

} // namespace

int main() {
    tests::support::TestResult result;

    static_assert(std::variant_size_v<typed::AskForApproval> == 3);
    static_assert(std::variant_size_v<typed::CommandAction> == 5);
    static_assert(std::variant_size_v<typed::DynamicToolCallOutputContentItem> == 3);
    static_assert(std::variant_size_v<typed::PatchChangeKind> == 4);
    static_assert(std::variant_size_v<typed::SandboxPolicy> == 5);
    static_assert(std::variant_size_v<typed::UserInput> == 6);
    static_assert(std::variant_size_v<typed::WebSearchAction> == 5);
    static_assert(std::is_same_v<typed::TurnInput, typed::UserInput>);

    const Json index = fixture("index.json");
    std::size_t positiveCount = 0;
    std::size_t bidirectionalCount = 0;
    std::size_t missingRequiredCount = 0;
    std::size_t wrongTypeCount = 0;
    std::size_t futureCount = 0;
    std::size_t optionalOmittedCount = 0;
    std::size_t nullableNullCount = 0;
    std::size_t bidirectionalSupplementCount = 0;
    std::size_t knownOpenEnumCount = 0;

    for (const Json& record : index.at("fixtures")) {
        const std::string id = record.at("id").get<std::string>();
        const std::string role = record.at("role").get<std::string>();
        const Json raw = fixture(record.at("file").get<std::string>());
        if (role == "open_enum_known_value") {
            const bool isB2OpenEnum = id.rfind("enum:ImageDetail:", 0) == 0 || id.rfind("enum:NetworkAccess:", 0) == 0;
            if (!isB2OpenEnum) {
                continue;
            }
            ++knownOpenEnumCount;
            std::string error;
            if (id.rfind("enum:ImageDetail:", 0) == 0) {
                const Json outer = {{"type", "image"}, {"url", "https://example.test/image"}, {"detail", raw}};
                const auto decoded = detail::decodeUserInput(outer);
                const auto* image = decoded.value ? std::get_if<typed::ImageUserInput>(&*decoded.value) : nullptr;
                const std::optional<Json> encoded = decoded.value ? detail::encodeUserInput(*decoded.value, error) : std::nullopt;
                result.expectTrue(image && image->detail.hasValue() && image->detail.value->value == raw.get<std::string>() &&
                                      image->detail.value->isKnown() && image->raw == outer && !decoded.diagnostic && encoded &&
                                      *encoded == outer && error.empty(),
                                  "indexed known ImageDetail value traverses the typed production decoder and encoder: " +
                                      raw.get<std::string>());
            } else if (id.rfind("enum:NetworkAccess:", 0) == 0) {
                const Json outer = {{"type", "externalSandbox"}, {"networkAccess", raw}};
                const auto decoded = detail::decodeSandboxPolicy(outer);
                const auto* sandbox = decoded.value ? std::get_if<typed::ExternalSandboxSandboxPolicy>(&*decoded.value) : nullptr;
                const std::optional<Json> encoded = decoded.value ? detail::encodeSandboxPolicy(*decoded.value, error) : std::nullopt;
                result.expectTrue(sandbox && sandbox->networkAccess && sandbox->networkAccess->value == raw.get<std::string>() &&
                                      sandbox->networkAccess->isKnown() && sandbox->raw == outer && !decoded.diagnostic && encoded &&
                                      *encoded == outer && error.empty(),
                                  "indexed known NetworkAccess value traverses the typed production decoder and encoder: " +
                                      raw.get<std::string>());
            }
            continue;
        }

        const auto key = record.find("protocol_surface_key");
        const std::string domain = key != record.end() && key->contains("domain")
                                       ? key->at("domain").get<std::string>()
                                       : (id.rfind("union:", 0) == 0 ? id.substr(6, id.find(':', 6) - 6) : std::string{});
        if (!isB2Domain(domain)) {
            continue;
        }

        const Observation observed = observe(domain, raw);

        if (role == "union_branch") {
            ++positiveCount;
            const std::string expected = key->at("name").get<std::string>();
            result.expectTrue(observed.hasValue && !observed.explicitUnknown && observed.label == expected && observed.rawRetained &&
                                  observed.fieldsMapped && !observed.diagnostic,
                              domain + " known fixture selects its exact typed alternative and maps every field: " + expected);
            if (domain == "AskForApproval" || domain == "SandboxPolicy" || domain == "UserInput") {
                ++bidirectionalCount;
                std::string error;
                const std::optional<Json> encoded = encodeBidirectional(domain, raw, error);
                result.expectTrue(encoded && *encoded == raw && error.empty(),
                                  domain + " bidirectional fixture re-encodes byte-semantically exactly: " + expected);
            }
        } else if (role == "malformed_known_missing_required" || role == "malformed_known_missing_discriminator") {
            ++missingRequiredCount;
            result.expectTrue(!observed.hasValue && isProtocolWarning(observed.diagnostic),
                              domain + " missing required field is exactly MalformedKnownPayload/ProtocolWarning");
        } else if (role == "malformed_known_wrong_type" || role == "malformed_known_wrong_discriminator_type") {
            ++wrongTypeCount;
            result.expectTrue(!observed.hasValue && isProtocolWarning(observed.diagnostic),
                              domain + " wrong nested type is exactly MalformedKnownPayload/ProtocolWarning");
        } else if (role == "unknown_discriminator") {
            ++futureCount;
            result.expectTrue(observed.hasValue && observed.explicitUnknown && observed.rawRetained && observed.fieldsMapped &&
                                  isForwardCompatible(observed.diagnostic),
                              domain + " future discriminator selects the explicit raw-retaining unknown alternative");
        } else if (role == "union_optional_omitted" || role == "union_nullable_null") {
            role == "union_optional_omitted" ? ++optionalOmittedCount : ++nullableNullCount;
            result.expectTrue(
                observed.hasValue && !observed.explicitUnknown && observed.rawRetained && observed.fieldsMapped && !observed.diagnostic,
                domain + " generated " + role + " fixture selects the typed alternative, maps every field, and retains raw JSON");
            if (domain == "AskForApproval" || domain == "SandboxPolicy" || domain == "UserInput") {
                ++bidirectionalSupplementCount;
                std::string error;
                const std::optional<Json> encoded = encodeBidirectional(domain, raw, error);
                result.expectTrue(encoded && *encoded == raw && error.empty(),
                                  domain + " generated " + role + " fixture re-encodes exactly");
            }
        }
    }

    result.expectTrue(positiveCount == 26, "indexed B2 corpus contains exactly 26 independently validated positive branches");
    result.expectTrue(bidirectionalCount == 13, "exactly 13 B2 alternatives are exercised bidirectionally");
    result.expectTrue(missingRequiredCount == 44,
                      "indexed B2 malformed matrix has exactly 44 non-discriminator plus discriminator removals");
    result.expectTrue(wrongTypeCount == 65, "indexed B2 malformed matrix has exactly 65 payload plus discriminator wrong-type cases");
    result.expectTrue(futureCount == 7, "indexed B2 corpus has one explicit future discriminator per union family");
    result.expectTrue(optionalOmittedCount == 21, "all 21 generated B2 optional-omitted fixtures exercise production decoders");
    result.expectTrue(nullableNullCount == 12, "all 12 generated B2 nullable-null fixtures exercise production decoders");
    result.expectTrue(bidirectionalSupplementCount == 15, "all 15 bidirectional B2 optional/null supplements re-encode exactly");
    result.expectTrue(knownOpenEnumCount == 6, "all six indexed known open-enum fixtures traverse production decode and encode paths");

    const auto omittedCommand =
        detail::decodeCommandAction(fixture("cases/unions/commandaction/supplements/search-optional-omitted-query.json"));
    const auto* omittedSearch = omittedCommand.value ? std::get_if<typed::SearchCommandAction>(&*omittedCommand.value) : nullptr;
    const auto nullCommand = detail::decodeCommandAction(fixture("cases/unions/commandaction/supplements/search-nullable-null-query.json"));
    const auto* nullSearch = nullCommand.value ? std::get_if<typed::SearchCommandAction>(&*nullCommand.value) : nullptr;
    const auto valueCommand = detail::decodeCommandAction(fixture("cases/unions/commandaction/search.json"));
    const auto* valueSearch = valueCommand.value ? std::get_if<typed::SearchCommandAction>(&*valueCommand.value) : nullptr;
    result.expectTrue(omittedSearch && omittedSearch->query.isOmitted() && nullSearch && nullSearch->query.isNull() && valueSearch &&
                          valueSearch->query.hasValue(),
                      "CommandAction preserves omitted/null/value as three distinct states");

    const auto omittedInput = detail::decodeUserInput(fixture("cases/unions/userinput/supplements/image-optional-omitted-detail.json"));
    const auto* omittedImage = omittedInput.value ? std::get_if<typed::ImageUserInput>(&*omittedInput.value) : nullptr;
    const auto nullInput = detail::decodeUserInput(fixture("cases/unions/userinput/supplements/image-nullable-null-detail.json"));
    const auto* nullImage = nullInput.value ? std::get_if<typed::ImageUserInput>(&*nullInput.value) : nullptr;
    const auto valueInput = detail::decodeUserInput(fixture("cases/unions/userinput/image.json"));
    const auto* valueImage = valueInput.value ? std::get_if<typed::ImageUserInput>(&*valueInput.value) : nullptr;
    result.expectTrue(omittedImage && omittedImage->detail.isOmitted() && nullImage && nullImage->detail.isNull() && valueImage &&
                          valueImage->detail.hasValue(),
                      "UserInput image detail preserves omitted/null/value as three distinct states");

    const Json futureImageDetail = {
        {"type", "image"},
        {"url", "https://example.test/secret-image"},
        {"detail", fixture("cases/enums/imagedetail/future-unknown.json")},
    };
    const auto futureImage = detail::decodeUserInput(futureImageDetail);
    const auto* typedFutureImage = futureImage.value ? std::get_if<typed::ImageUserInput>(&*futureImage.value) : nullptr;
    result.expectTrue(typedFutureImage && typedFutureImage->detail.hasValue() &&
                          typedFutureImage->detail.value->value ==
                              fixture("cases/enums/imagedetail/future-unknown.json").get<std::string>() &&
                          typedFutureImage->raw == futureImageDetail &&
                          isForwardCompatible(futureImage.diagnostic, typed::DecodeIssueKind::UnknownEnumValue) &&
                          futureImage.diagnostic->fieldPath == "$.detail",
                      "future ImageDetail stays in the typed outer UserInput with UnknownEnumValue");
    std::string futureEnumEncodeError;
    const std::optional<Json> reencodedFutureImage =
        futureImage.value ? detail::encodeUserInput(*futureImage.value, futureEnumEncodeError) : std::nullopt;
    result.expectTrue(reencodedFutureImage == futureImageDetail && futureEnumEncodeError.empty(),
                      "future ImageDetail round-trips through the bidirectional open-string encoder");

    const Json futureNetwork = {
        {"type", "externalSandbox"},
        {"networkAccess", fixture("cases/enums/networkaccess/future-unknown.json")},
    };
    const auto futureSandbox = detail::decodeSandboxPolicy(futureNetwork);
    const auto* typedFutureSandbox =
        futureSandbox.value ? std::get_if<typed::ExternalSandboxSandboxPolicy>(&*futureSandbox.value) : nullptr;
    result.expectTrue(typedFutureSandbox && typedFutureSandbox->networkAccess &&
                          typedFutureSandbox->networkAccess->value ==
                              fixture("cases/enums/networkaccess/future-unknown.json").get<std::string>() &&
                          typedFutureSandbox->raw == futureNetwork &&
                          isForwardCompatible(futureSandbox.diagnostic, typed::DecodeIssueKind::UnknownEnumValue) &&
                          futureSandbox.diagnostic->fieldPath == "$.networkAccess",
                      "future NetworkAccess stays in the typed outer SandboxPolicy with UnknownEnumValue");
    const std::optional<Json> reencodedFutureSandbox =
        futureSandbox.value ? detail::encodeSandboxPolicy(*futureSandbox.value, futureEnumEncodeError) : std::nullopt;
    result.expectTrue(reencodedFutureSandbox == futureNetwork && futureEnumEncodeError.empty(),
                      "future NetworkAccess round-trips through the bidirectional open-string encoder");

    const Json emptyImageDetail = {{"type", "image"}, {"url", "https://example.test/image"}, {"detail", ""}};
    const auto emptyImage = detail::decodeUserInput(emptyImageDetail);
    const auto* typedEmptyImage = emptyImage.value ? std::get_if<typed::ImageUserInput>(&*emptyImage.value) : nullptr;
    const std::optional<Json> reencodedEmptyImage =
        emptyImage.value ? detail::encodeUserInput(*emptyImage.value, futureEnumEncodeError) : std::nullopt;
    result.expectTrue(typedEmptyImage && typedEmptyImage->detail.hasValue() && typedEmptyImage->detail.value->value.empty() &&
                          !typedEmptyImage->detail.value->isKnown() &&
                          isForwardCompatible(emptyImage.diagnostic, typed::DecodeIssueKind::UnknownEnumValue) &&
                          reencodedEmptyImage == emptyImageDetail && futureEnumEncodeError.empty(),
                      "empty future ImageDetail remains an open string value and round-trips exactly");

    const Json emptyNetworkAccess = {{"type", "externalSandbox"}, {"networkAccess", ""}};
    const auto emptySandbox = detail::decodeSandboxPolicy(emptyNetworkAccess);
    const auto* typedEmptySandbox = emptySandbox.value ? std::get_if<typed::ExternalSandboxSandboxPolicy>(&*emptySandbox.value) : nullptr;
    const std::optional<Json> reencodedEmptySandbox =
        emptySandbox.value ? detail::encodeSandboxPolicy(*emptySandbox.value, futureEnumEncodeError) : std::nullopt;
    result.expectTrue(typedEmptySandbox && typedEmptySandbox->networkAccess && typedEmptySandbox->networkAccess->value.empty() &&
                          !typedEmptySandbox->networkAccess->isKnown() &&
                          isForwardCompatible(emptySandbox.diagnostic, typed::DecodeIssueKind::UnknownEnumValue) &&
                          reencodedEmptySandbox == emptyNetworkAccess && futureEnumEncodeError.empty(),
                      "empty future NetworkAccess remains an open string value and round-trips exactly");

    const Json conflictingAsk = {
        {"granular", {{"mcp_elicitations", true}, {"rules", true}, {"sandbox_approval", true}}},
        {"futureAskForApproval", Json::object()},
    };
    const auto conflicting = detail::decodeAskForApproval(conflictingAsk);
    result.expectTrue(!conflicting.value && isProtocolWarning(conflicting.diagnostic) && conflicting.diagnostic->fieldPath == "$",
                      "multiple externally tagged AskForApproval discriminators are malformed-known, not future");

    constexpr std::string_view Sensitive = "secret-token-value";
    const Json sensitiveRaw = {
        {"type", "read"},
        {"command", Sensitive},
        {"name", "cat"},
        {"path", Json::array()},
    };
    const auto sensitive = detail::decodeCommandAction(sensitiveRaw);
    result.expectTrue(!sensitive.value && isProtocolWarning(sensitive.diagnostic) &&
                          omitsSensitiveValue(*sensitive.diagnostic, Sensitive) && sensitiveRaw.dump().find(Sensitive) != std::string::npos,
                      "malformed diagnostics contain identity/path only while raw input retains the sensitive value");

    std::string encodeError;
    const std::optional<Json> unknownEncoding =
        detail::encodeUserInput(typed::UserInput{typed::UnknownUserInput{.type = "future", .raw = {{"type", "future"}}}}, encodeError);
    result.expectTrue(!unknownEncoding && !encodeError.empty(),
                      "outgoing explicit unknown UserInput is rejected synchronously in favor of the raw API");

    typed::ImageUserInput inconsistent{
        .url = "https://example.test/image",
        .detail = typed::OptionalNullable<typed::ImageDetail>::omitted(),
    };
    inconsistent.detail.value = typed::ImageDetail::high();
    encodeError.clear();
    const std::optional<Json> inconsistentEncoding = detail::encodeUserInput(typed::UserInput{inconsistent}, encodeError);
    result.expectTrue(!inconsistentEncoding && !encodeError.empty(),
                      "an internally inconsistent omitted-plus-value tri-state is rejected synchronously");

    const std::span<const detail::ConversationUnionCodecDescriptor> descriptors = detail::conversationUnionCodecDescriptors();
    result.expectTrue(descriptors.size() == 58,
                      "generated production descriptor set contains exactly 26 B2, 16 dependency-closed B3, and 16 B4 targets");
    bool descriptorAgreement = true;
    for (const auto& descriptor : descriptors) {
        const detail::ProtocolSurfaceEntry* row =
            detail::findSurface(descriptor.key.category, descriptor.key.domain, descriptor.key.field, descriptor.key.name);
        const auto* target = row ? std::get_if<detail::ConversationUnionTarget>(&row->runtimeTarget) : nullptr;
        descriptorAgreement = descriptorAgreement && row && row->stability == detail::Stability::Stable &&
                              row->runtimeDisposition == detail::RuntimeDisposition::Typed &&
                              row->typedImplementation == detail::TypedImplementationStatus::Implemented &&
                              row->typedSchemaStatus == detail::TypedSchemaStatus::Complete &&
                              detail::derivedTypedSchemaStatus(*row) == detail::TypedSchemaStatus::Complete && target &&
                              *target == descriptor.target && &detail::entryFor(descriptor.target) == row;
    }
    result.expectTrue(descriptorAgreement, "every generated descriptor agrees bidirectionally with one complete canonical registry row");

    return result.processResult();
}
