/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Items.h"
#include "support/TestResult.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace {
    using ai::openai::codex::Json;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    const std::filesystem::path FixtureRoot = CODEX_A1_FIXTURE_ROOT;

    Json fixture(const std::filesystem::path& relative) {
        std::ifstream input(FixtureRoot / relative);
        return Json::parse(input);
    }

    bool isProtocolWarning(const std::optional<typed::DecodeDiagnostic>& diagnostic, std::string_view surface, std::string_view path) {
        return diagnostic && diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
               diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning && diagnostic->surface == surface &&
               diagnostic->fieldPath == path;
    }

    bool isForwardCompatible(const std::optional<typed::DecodeDiagnostic>& diagnostic,
                             typed::DecodeIssueKind kind,
                             std::string_view surface,
                             std::string_view path) {
        return diagnostic && diagnostic->kind == kind && diagnostic->severity == typed::DecodeIssueSeverity::ForwardCompatibility &&
               diagnostic->surface == surface && diagnostic->fieldPath == path;
    }

    template <typename T>
    bool
    nullableStringMatches(const typed::OptionalNullable<T>& decoded, const Json& source, std::string_view field, const auto& toString) {
        const auto iterator = source.find(field);
        if (iterator == source.end()) {
            return decoded.isOmitted();
        }
        if (iterator->is_null()) {
            return decoded.isNull();
        }
        return decoded.hasValue() && iterator->is_string() && toString(*decoded.value) == iterator->get<std::string>();
    }

    template <typename Decode>
    bool malformed(Decode&& decode, const Json& value, std::string_view surface, std::string_view path) {
        const auto result = decode(value);
        return !result.value && isProtocolWarning(result.diagnostic, surface, path);
    }

    template <typename Unknown, typename Decode>
    bool explicitUnknown(Decode&& decode, const Json& raw, std::string_view surface) {
        const auto result = decode(raw);
        const auto* unknown = result.value ? std::get_if<Unknown>(&*result.value) : nullptr;
        return unknown && unknown->type && unknown->raw == raw &&
               isForwardCompatible(result.diagnostic, typed::DecodeIssueKind::UnknownDiscriminator, surface, "$.type") &&
               unknown->diagnostic == result.diagnostic;
    }

    template <typename Alternative, typename Decode, typename Member>
    bool triStateFixtures(Decode&& decode,
                          const std::filesystem::path& omittedFile,
                          const std::filesystem::path& nullFile,
                          const std::filesystem::path& valueFile,
                          Member member) {
        const Json omittedRaw = fixture(omittedFile);
        const Json nullRaw = fixture(nullFile);
        const Json valueRaw = fixture(valueFile);
        const auto omittedResult = decode(omittedRaw);
        const auto nullResult = decode(nullRaw);
        const auto valueResult = decode(valueRaw);
        const auto* omitted = omittedResult.value ? std::get_if<Alternative>(&*omittedResult.value) : nullptr;
        const auto* nullValue = nullResult.value ? std::get_if<Alternative>(&*nullResult.value) : nullptr;
        const auto* typedValue = valueResult.value ? std::get_if<Alternative>(&*valueResult.value) : nullptr;
        return omitted && (omitted->*member).isOmitted() && omitted->raw == omittedRaw && !omittedResult.diagnostic && nullValue &&
               (nullValue->*member).isNull() && nullValue->raw == nullRaw && !nullResult.diagnostic && typedValue &&
               (typedValue->*member).hasValue() && typedValue->raw == valueRaw && !valueResult.diagnostic;
    }

    template <typename Decode, typename Case, std::size_t Size>
    bool allMalformed(Decode&& decode, std::string_view surface, const std::array<Case, Size>& cases) {
        for (const auto& [file, path] : cases) {
            if (!malformed(decode, fixture(file), surface, path)) {
                return false;
            }
        }
        return true;
    }

    template <typename Decode>
    std::pair<std::size_t, std::size_t>
    indexedDiscriminatorFailures(const Json& index, std::string_view domain, Decode&& decode, bool& exact) {
        std::size_t missing = 0;
        std::size_t wrongType = 0;
        for (const Json& record : index.at("fixtures")) {
            const auto key = record.find("protocol_surface_key");
            if (key == record.end() || key->value("domain", "") != domain) {
                continue;
            }
            const std::string role = record.value("role", "");
            if (role != "malformed_known_missing_discriminator" &&
                role != "malformed_known_wrong_discriminator_type") {
                continue;
            }
            missing += role == "malformed_known_missing_discriminator";
            wrongType += role == "malformed_known_wrong_discriminator_type";
            const auto decoded = decode(fixture(record.at("file").get<std::string>()));
            exact = exact && !decoded.value && isProtocolWarning(decoded.diagnostic, domain, "$.type");
        }
        return {missing, wrongType};
    }

    template <typename Decode>
    std::size_t indexedNestedOpenEnums(const Json& index, std::string_view domain, Decode&& decode, bool& exact) {
        std::size_t count = 0;
        for (const Json& record : index.at("fixtures")) {
            const auto key = record.find("protocol_surface_key");
            if (key == record.end() || key->value("domain", "") != domain || record.value("role", "") != "unknown_enum_value") {
                continue;
            }
            const Json raw = fixture(record.at("file").get<std::string>());
            const auto decoded = decode(raw);
            bool knownOuterWithRaw = false;
            if (decoded.value) {
                knownOuterWithRaw = decoded.value->index() + 1 < std::variant_size_v<typename decltype(decoded.value)::value_type> &&
                                    std::visit([&raw](const auto& value) { return value.raw == raw; }, *decoded.value);
            }
            exact = exact && knownOuterWithRaw &&
                    isForwardCompatible(decoded.diagnostic, typed::DecodeIssueKind::UnknownEnumValue, domain, "$.detail");
            ++count;
        }
        return count;
    }

} // namespace

int main() {
    tests::support::TestResult result;
    const Json fixtureIndex = fixture("index.json");

    bool allDiscriminatorFailuresExact = true;
    std::size_t missingDiscriminators = 0;
    std::size_t wrongDiscriminatorTypes = 0;
    const auto addDiscriminatorFailures = [&](const auto& counts) {
        missingDiscriminators += counts.first;
        wrongDiscriminatorTypes += counts.second;
    };
    addDiscriminatorFailures(indexedDiscriminatorFailures(
        fixtureIndex, "AgentMessageInputContent", detail::decodeAgentMessageInputContent, allDiscriminatorFailuresExact));
    addDiscriminatorFailures(
        indexedDiscriminatorFailures(fixtureIndex, "ContentItem", detail::decodeContentItem, allDiscriminatorFailuresExact));
    addDiscriminatorFailures(indexedDiscriminatorFailures(fixtureIndex,
                                                          "FunctionCallOutputContentItem",
                                                          detail::decodeFunctionCallOutputContentItem,
                                                          allDiscriminatorFailuresExact));
    addDiscriminatorFailures(
        indexedDiscriminatorFailures(fixtureIndex, "LocalShellAction", detail::decodeLocalShellAction, allDiscriminatorFailuresExact));
    addDiscriminatorFailures(indexedDiscriminatorFailures(
        fixtureIndex, "ReasoningItemContent", detail::decodeReasoningItemContent, allDiscriminatorFailuresExact));
    addDiscriminatorFailures(indexedDiscriminatorFailures(fixtureIndex,
                                                          "ReasoningItemReasoningSummary",
                                                          detail::decodeReasoningItemReasoningSummary,
                                                          allDiscriminatorFailuresExact));
    addDiscriminatorFailures(indexedDiscriminatorFailures(fixtureIndex,
                                                          "ResponsesApiWebSearchAction",
                                                          detail::decodeResponsesApiWebSearchAction,
                                                          allDiscriminatorFailuresExact));
    result.expectTrue(missingDiscriminators == 16 && wrongDiscriminatorTypes == 16 && allDiscriminatorFailuresExact,
                      "all 16 B3 nested alternatives classify missing and wrong-type discriminators as exact malformed-known warnings");

    bool allNestedOpenEnumsExact = true;
    const std::size_t nestedOpenEnums =
        indexedNestedOpenEnums(fixtureIndex, "ContentItem", detail::decodeContentItem, allNestedOpenEnumsExact) +
        indexedNestedOpenEnums(fixtureIndex,
                               "FunctionCallOutputContentItem",
                               detail::decodeFunctionCallOutputContentItem,
                               allNestedOpenEnumsExact);
    result.expectTrue(nestedOpenEnums == 4 && allNestedOpenEnumsExact,
                      "future and empty ImageDetail values remain typed in both direction-specific content unions");

    Json shellBounds = fixture("cases/unions/localshellaction/exec.json");
    shellBounds["timeout_ms"] = std::numeric_limits<std::uint64_t>::max();
    const auto maximumShell = detail::decodeLocalShellAction(shellBounds);
    const auto* maximumShellExec =
        maximumShell.value ? std::get_if<typed::ExecLocalShellAction>(&*maximumShell.value) : nullptr;
    result.expectTrue(maximumShellExec && maximumShellExec->timeoutMs.hasValue() &&
                          *maximumShellExec->timeoutMs == std::numeric_limits<std::uint64_t>::max(),
                      "LocalShellAction accepts the exact uint64 timeout upper bound");
    shellBounds["timeout_ms"] = -1;
    const auto negativeShell = detail::decodeLocalShellAction(shellBounds);
    shellBounds["timeout_ms"] = 1.5;
    const auto fractionalShell = detail::decodeLocalShellAction(shellBounds);
    result.expectTrue(!negativeShell.value && isProtocolWarning(negativeShell.diagnostic, "LocalShellAction", "$.timeout_ms") &&
                          !fractionalShell.value &&
                          isProtocolWarning(fractionalShell.diagnostic, "LocalShellAction", "$.timeout_ms"),
                      "LocalShellAction rejects negative and fractional uint64 timeouts at the exact field path");

    const Json agentTextRaw = fixture("cases/unions/agentmessageinputcontent/input_text.json");
    const auto agentTextDecoded = detail::decodeAgentMessageInputContent(agentTextRaw);
    const auto* agentText =
        agentTextDecoded.value ? std::get_if<typed::InputTextAgentMessageInputContent>(&*agentTextDecoded.value) : nullptr;
    result.expectTrue(agentText && agentText->text == agentTextRaw.at("text").get<std::string>() && agentText->raw == agentTextRaw &&
                          !agentTextDecoded.diagnostic,
                      "AgentMessageInputContent input_text maps every field and retains raw JSON");

    const Json agentEncryptedRaw = fixture("cases/unions/agentmessageinputcontent/encrypted_content.json");
    const auto agentEncryptedDecoded = detail::decodeAgentMessageInputContent(agentEncryptedRaw);
    const auto* agentEncrypted =
        agentEncryptedDecoded.value ? std::get_if<typed::EncryptedContentAgentMessageInputContent>(&*agentEncryptedDecoded.value) : nullptr;
    result.expectTrue(agentEncrypted && agentEncrypted->encryptedContent == agentEncryptedRaw.at("encrypted_content").get<std::string>() &&
                          agentEncrypted->raw == agentEncryptedRaw && !agentEncryptedDecoded.diagnostic,
                      "AgentMessageInputContent encrypted_content maps every field and retains raw JSON");

    const Json contentTextRaw = fixture("cases/unions/contentitem/input_text.json");
    const auto contentTextDecoded = detail::decodeContentItem(contentTextRaw);
    const auto* contentText = contentTextDecoded.value ? std::get_if<typed::InputTextContentItem>(&*contentTextDecoded.value) : nullptr;
    result.expectTrue(contentText && contentText->text == contentTextRaw.at("text").get<std::string>() &&
                          contentText->raw == contentTextRaw && !contentTextDecoded.diagnostic,
                      "ContentItem input_text maps every field and retains raw JSON");

    const Json contentImageRaw = fixture("cases/unions/contentitem/input_image.json");
    const auto contentImageDecoded = detail::decodeContentItem(contentImageRaw);
    const auto* contentImage = contentImageDecoded.value ? std::get_if<typed::InputImageContentItem>(&*contentImageDecoded.value) : nullptr;
    result.expectTrue(contentImage && contentImage->imageUrl == contentImageRaw.at("image_url").get<std::string>() &&
                          nullableStringMatches(contentImage->detail,
                                                contentImageRaw,
                                                "detail",
                                                [](const typed::ImageDetail& detail) {
                                                    return detail.value;
                                                }) &&
                          contentImage->raw == contentImageRaw && !contentImageDecoded.diagnostic,
                      "ContentItem input_image maps every field and retains raw JSON");

    const Json contentOutputRaw = fixture("cases/unions/contentitem/output_text.json");
    const auto contentOutputDecoded = detail::decodeContentItem(contentOutputRaw);
    const auto* contentOutput =
        contentOutputDecoded.value ? std::get_if<typed::OutputTextContentItem>(&*contentOutputDecoded.value) : nullptr;
    result.expectTrue(contentOutput && contentOutput->text == contentOutputRaw.at("text").get<std::string>() &&
                          contentOutput->raw == contentOutputRaw && !contentOutputDecoded.diagnostic,
                      "ContentItem output_text maps every field and retains raw JSON");

    const Json functionTextRaw = fixture("cases/unions/functioncalloutputcontentitem/input_text.json");
    const auto functionTextDecoded = detail::decodeFunctionCallOutputContentItem(functionTextRaw);
    const auto* functionText =
        functionTextDecoded.value ? std::get_if<typed::InputTextFunctionCallOutputContentItem>(&*functionTextDecoded.value) : nullptr;
    result.expectTrue(functionText && functionText->text == functionTextRaw.at("text").get<std::string>() &&
                          functionText->raw == functionTextRaw && !functionTextDecoded.diagnostic,
                      "FunctionCallOutputContentItem input_text maps every field and retains raw JSON");

    const Json functionImageRaw = fixture("cases/unions/functioncalloutputcontentitem/input_image.json");
    const auto functionImageDecoded = detail::decodeFunctionCallOutputContentItem(functionImageRaw);
    const auto* functionImage =
        functionImageDecoded.value ? std::get_if<typed::InputImageFunctionCallOutputContentItem>(&*functionImageDecoded.value) : nullptr;
    result.expectTrue(functionImage && functionImage->imageUrl == functionImageRaw.at("image_url").get<std::string>() &&
                          nullableStringMatches(functionImage->detail,
                                                functionImageRaw,
                                                "detail",
                                                [](const typed::ImageDetail& detail) {
                                                    return detail.value;
                                                }) &&
                          functionImage->raw == functionImageRaw && !functionImageDecoded.diagnostic,
                      "FunctionCallOutputContentItem input_image maps every field and retains raw JSON");

    const Json functionEncryptedRaw = fixture("cases/unions/functioncalloutputcontentitem/encrypted_content.json");
    const auto functionEncryptedDecoded = detail::decodeFunctionCallOutputContentItem(functionEncryptedRaw);
    const auto* functionEncrypted =
        functionEncryptedDecoded.value ? std::get_if<typed::EncryptedContentFunctionCallOutputContentItem>(&*functionEncryptedDecoded.value)
                                       : nullptr;
    result.expectTrue(functionEncrypted &&
                          functionEncrypted->encryptedContent == functionEncryptedRaw.at("encrypted_content").get<std::string>() &&
                          functionEncrypted->raw == functionEncryptedRaw && !functionEncryptedDecoded.diagnostic,
                      "FunctionCallOutputContentItem encrypted_content maps every field and retains raw JSON");

    const Json shellRaw = fixture("cases/unions/localshellaction/exec.json");
    const auto shellDecoded = detail::decodeLocalShellAction(shellRaw);
    const auto* shell = shellDecoded.value ? std::get_if<typed::ExecLocalShellAction>(&*shellDecoded.value) : nullptr;
    result.expectTrue(shell && shell->command.size() == shellRaw.at("command").size() &&
                          shell->command.front() == shellRaw.at("command").front().get<std::string>() && shell->env.hasValue() &&
                          shell->env.value->at("syntheticKey") == shellRaw.at("env").at("syntheticKey").get<std::string>() &&
                          shell->timeoutMs.hasValue() && *shell->timeoutMs == shellRaw.at("timeout_ms").get<std::uint64_t>() &&
                          nullableStringMatches(shell->user,
                                                shellRaw,
                                                "user",
                                                [](const std::string& value) {
                                                    return value;
                                                }) &&
                          nullableStringMatches(shell->workingDirectory,
                                                shellRaw,
                                                "working_directory",
                                                [](const std::string& value) {
                                                    return value;
                                                }) &&
                          shell->raw == shellRaw && !shellDecoded.diagnostic,
                      "LocalShellAction exec maps every field and retains raw JSON");

    const Json reasoningTextRaw = fixture("cases/unions/reasoningitemcontent/reasoning_text.json");
    const auto reasoningTextDecoded = detail::decodeReasoningItemContent(reasoningTextRaw);
    const auto* reasoningText =
        reasoningTextDecoded.value ? std::get_if<typed::ReasoningTextReasoningItemContent>(&*reasoningTextDecoded.value) : nullptr;
    result.expectTrue(reasoningText && reasoningText->text == reasoningTextRaw.at("text").get<std::string>() &&
                          reasoningText->raw == reasoningTextRaw && !reasoningTextDecoded.diagnostic,
                      "ReasoningItemContent reasoning_text maps every field and retains raw JSON");

    const Json plainReasoningRaw = fixture("cases/unions/reasoningitemcontent/text.json");
    const auto plainReasoningDecoded = detail::decodeReasoningItemContent(plainReasoningRaw);
    const auto* plainReasoning =
        plainReasoningDecoded.value ? std::get_if<typed::TextReasoningItemContent>(&*plainReasoningDecoded.value) : nullptr;
    result.expectTrue(plainReasoning && plainReasoning->text == plainReasoningRaw.at("text").get<std::string>() &&
                          plainReasoning->raw == plainReasoningRaw && !plainReasoningDecoded.diagnostic,
                      "ReasoningItemContent text maps every field and retains raw JSON");

    const Json summaryRaw = fixture("cases/unions/reasoningitemreasoningsummary/summary_text.json");
    const auto summaryDecoded = detail::decodeReasoningItemReasoningSummary(summaryRaw);
    const auto* summary =
        summaryDecoded.value ? std::get_if<typed::SummaryTextReasoningItemReasoningSummary>(&*summaryDecoded.value) : nullptr;
    result.expectTrue(summary && summary->text == summaryRaw.at("text").get<std::string>() && summary->raw == summaryRaw &&
                          !summaryDecoded.diagnostic,
                      "ReasoningItemReasoningSummary summary_text maps every field and retains raw JSON");

    const Json responseSearchRaw = fixture("cases/unions/responsesapiwebsearchaction/search.json");
    const auto responseSearchDecoded = detail::decodeResponsesApiWebSearchAction(responseSearchRaw);
    const auto* responseSearch =
        responseSearchDecoded.value ? std::get_if<typed::SearchResponsesApiWebSearchAction>(&*responseSearchDecoded.value) : nullptr;
    result.expectTrue(responseSearch && responseSearch->queries.hasValue() &&
                          *responseSearch->queries == responseSearchRaw.at("queries").get<std::vector<std::string>>() &&
                          nullableStringMatches(responseSearch->query,
                                                responseSearchRaw,
                                                "query",
                                                [](const std::string& value) {
                                                    return value;
                                                }) &&
                          responseSearch->raw == responseSearchRaw && !responseSearchDecoded.diagnostic,
                      "ResponsesApiWebSearchAction search maps every field and retains raw JSON");

    const Json responseOpenRaw = fixture("cases/unions/responsesapiwebsearchaction/open_page.json");
    const auto responseOpenDecoded = detail::decodeResponsesApiWebSearchAction(responseOpenRaw);
    const auto* responseOpen =
        responseOpenDecoded.value ? std::get_if<typed::OpenPageResponsesApiWebSearchAction>(&*responseOpenDecoded.value) : nullptr;
    result.expectTrue(responseOpen &&
                          nullableStringMatches(responseOpen->url,
                                                responseOpenRaw,
                                                "url",
                                                [](const std::string& value) {
                                                    return value;
                                                }) &&
                          responseOpen->raw == responseOpenRaw && !responseOpenDecoded.diagnostic,
                      "ResponsesApiWebSearchAction open_page maps every field and retains raw JSON");

    const Json responseFindRaw = fixture("cases/unions/responsesapiwebsearchaction/find_in_page.json");
    const auto responseFindDecoded = detail::decodeResponsesApiWebSearchAction(responseFindRaw);
    const auto* responseFind =
        responseFindDecoded.value ? std::get_if<typed::FindInPageResponsesApiWebSearchAction>(&*responseFindDecoded.value) : nullptr;
    result.expectTrue(responseFind &&
                          nullableStringMatches(responseFind->pattern,
                                                responseFindRaw,
                                                "pattern",
                                                [](const std::string& value) {
                                                    return value;
                                                }) &&
                          nullableStringMatches(responseFind->url,
                                                responseFindRaw,
                                                "url",
                                                [](const std::string& value) {
                                                    return value;
                                                }) &&
                          responseFind->raw == responseFindRaw && !responseFindDecoded.diagnostic,
                      "ResponsesApiWebSearchAction find_in_page maps every field and retains raw JSON");

    const Json responseOtherRaw = fixture("cases/unions/responsesapiwebsearchaction/other.json");
    const auto responseOtherDecoded = detail::decodeResponsesApiWebSearchAction(responseOtherRaw);
    const auto* responseOther =
        responseOtherDecoded.value ? std::get_if<typed::OtherResponsesApiWebSearchAction>(&*responseOtherDecoded.value) : nullptr;
    result.expectTrue(responseOther && responseOther->raw == responseOtherRaw && !responseOtherDecoded.diagnostic,
                      "ResponsesApiWebSearchAction other is a distinct known alternative");

    const Json contentOmittedRaw = fixture("cases/unions/contentitem/supplements/input_image-optional-omitted-detail.json");
    const Json contentNullRaw = fixture("cases/unions/contentitem/supplements/input_image-nullable-null-detail.json");
    const auto contentOmittedDecoded = detail::decodeContentItem(contentOmittedRaw);
    const auto contentNullDecoded = detail::decodeContentItem(contentNullRaw);
    const auto* contentOmitted =
        contentOmittedDecoded.value ? std::get_if<typed::InputImageContentItem>(&*contentOmittedDecoded.value) : nullptr;
    const auto* contentNull = contentNullDecoded.value ? std::get_if<typed::InputImageContentItem>(&*contentNullDecoded.value) : nullptr;
    result.expectTrue(contentOmitted && contentOmitted->detail.isOmitted() && contentNull && contentNull->detail.isNull() && contentImage &&
                          contentImage->detail.hasValue(),
                      "ContentItem detail preserves omitted/null/value");

    const Json functionOmittedRaw =
        fixture("cases/unions/functioncalloutputcontentitem/supplements/input_image-optional-omitted-detail.json");
    const Json functionNullRaw = fixture("cases/unions/functioncalloutputcontentitem/supplements/input_image-nullable-null-detail.json");
    const auto functionOmittedDecoded = detail::decodeFunctionCallOutputContentItem(functionOmittedRaw);
    const auto functionNullDecoded = detail::decodeFunctionCallOutputContentItem(functionNullRaw);
    const auto* functionOmitted = functionOmittedDecoded.value
                                      ? std::get_if<typed::InputImageFunctionCallOutputContentItem>(&*functionOmittedDecoded.value)
                                      : nullptr;
    const auto* functionNull =
        functionNullDecoded.value ? std::get_if<typed::InputImageFunctionCallOutputContentItem>(&*functionNullDecoded.value) : nullptr;
    result.expectTrue(functionOmitted && functionOmitted->detail.isOmitted() && functionNull && functionNull->detail.isNull() &&
                          functionImage && functionImage->detail.hasValue(),
                      "FunctionCallOutputContentItem detail preserves omitted/null/value");

    const auto shellOmitted =
        detail::decodeLocalShellAction(fixture("cases/unions/localshellaction/supplements/exec-optional-omitted-env.json"));
    const auto shellNull = detail::decodeLocalShellAction(fixture("cases/unions/localshellaction/supplements/exec-nullable-null-env.json"));
    const auto* omittedExec = shellOmitted.value ? std::get_if<typed::ExecLocalShellAction>(&*shellOmitted.value) : nullptr;
    const auto* nullExec = shellNull.value ? std::get_if<typed::ExecLocalShellAction>(&*shellNull.value) : nullptr;
    result.expectTrue(omittedExec && omittedExec->env.isOmitted() && nullExec && nullExec->env.isNull() && shell && shell->env.hasValue(),
                      "LocalShellAction env preserves omitted/null/value");

    const auto responseOmitted = detail::decodeResponsesApiWebSearchAction(
        fixture("cases/unions/responsesapiwebsearchaction/supplements/search-optional-omitted-query.json"));
    const auto responseNull = detail::decodeResponsesApiWebSearchAction(
        fixture("cases/unions/responsesapiwebsearchaction/supplements/search-nullable-null-query.json"));
    const auto* omittedSearch =
        responseOmitted.value ? std::get_if<typed::SearchResponsesApiWebSearchAction>(&*responseOmitted.value) : nullptr;
    const auto* nullSearch = responseNull.value ? std::get_if<typed::SearchResponsesApiWebSearchAction>(&*responseNull.value) : nullptr;
    result.expectTrue(omittedSearch && omittedSearch->query.isOmitted() && nullSearch && nullSearch->query.isNull() && responseSearch &&
                          responseSearch->query.hasValue(),
                      "ResponsesApiWebSearchAction query preserves omitted/null/value");

    result.expectTrue(
        triStateFixtures<typed::InputImageContentItem>(detail::decodeContentItem,
                                                       "cases/unions/contentitem/supplements/input_image-optional-omitted-detail.json",
                                                       "cases/unions/contentitem/supplements/input_image-nullable-null-detail.json",
                                                       "cases/unions/contentitem/input_image.json",
                                                       &typed::InputImageContentItem::detail) &&
            triStateFixtures<typed::InputImageFunctionCallOutputContentItem>(
                detail::decodeFunctionCallOutputContentItem,
                "cases/unions/functioncalloutputcontentitem/supplements/input_image-optional-omitted-detail.json",
                "cases/unions/functioncalloutputcontentitem/supplements/input_image-nullable-null-detail.json",
                "cases/unions/functioncalloutputcontentitem/input_image.json",
                &typed::InputImageFunctionCallOutputContentItem::detail),
        "both direction-specific image-content detail fields exercise omitted/null/value with raw retention");

    result.expectTrue(
        triStateFixtures<typed::ExecLocalShellAction>(detail::decodeLocalShellAction,
                                                      "cases/unions/localshellaction/supplements/exec-optional-omitted-env.json",
                                                      "cases/unions/localshellaction/supplements/exec-nullable-null-env.json",
                                                      "cases/unions/localshellaction/exec.json",
                                                      &typed::ExecLocalShellAction::env) &&
            triStateFixtures<typed::ExecLocalShellAction>(detail::decodeLocalShellAction,
                                                          "cases/unions/localshellaction/supplements/exec-optional-omitted-timeout_ms.json",
                                                          "cases/unions/localshellaction/supplements/exec-nullable-null-timeout_ms.json",
                                                          "cases/unions/localshellaction/exec.json",
                                                          &typed::ExecLocalShellAction::timeoutMs) &&
            triStateFixtures<typed::ExecLocalShellAction>(detail::decodeLocalShellAction,
                                                          "cases/unions/localshellaction/supplements/exec-optional-omitted-user.json",
                                                          "cases/unions/localshellaction/supplements/exec-nullable-null-user.json",
                                                          "cases/unions/localshellaction/exec.json",
                                                          &typed::ExecLocalShellAction::user) &&
            triStateFixtures<typed::ExecLocalShellAction>(
                detail::decodeLocalShellAction,
                "cases/unions/localshellaction/supplements/exec-optional-omitted-working_directory.json",
                "cases/unions/localshellaction/supplements/exec-nullable-null-working_directory.json",
                "cases/unions/localshellaction/exec.json",
                &typed::ExecLocalShellAction::workingDirectory),
        "all four LocalShellAction optional-nullable fields exercise omitted/null/value with raw retention");

    result.expectTrue(triStateFixtures<typed::SearchResponsesApiWebSearchAction>(
                          detail::decodeResponsesApiWebSearchAction,
                          "cases/unions/responsesapiwebsearchaction/supplements/search-optional-omitted-queries.json",
                          "cases/unions/responsesapiwebsearchaction/supplements/search-nullable-null-queries.json",
                          "cases/unions/responsesapiwebsearchaction/search.json",
                          &typed::SearchResponsesApiWebSearchAction::queries) &&
                          triStateFixtures<typed::SearchResponsesApiWebSearchAction>(
                              detail::decodeResponsesApiWebSearchAction,
                              "cases/unions/responsesapiwebsearchaction/supplements/search-optional-omitted-query.json",
                              "cases/unions/responsesapiwebsearchaction/supplements/search-nullable-null-query.json",
                              "cases/unions/responsesapiwebsearchaction/search.json",
                              &typed::SearchResponsesApiWebSearchAction::query) &&
                          triStateFixtures<typed::OpenPageResponsesApiWebSearchAction>(
                              detail::decodeResponsesApiWebSearchAction,
                              "cases/unions/responsesapiwebsearchaction/supplements/open_page-optional-omitted-url.json",
                              "cases/unions/responsesapiwebsearchaction/supplements/open_page-nullable-null-url.json",
                              "cases/unions/responsesapiwebsearchaction/open_page.json",
                              &typed::OpenPageResponsesApiWebSearchAction::url) &&
                          triStateFixtures<typed::FindInPageResponsesApiWebSearchAction>(
                              detail::decodeResponsesApiWebSearchAction,
                              "cases/unions/responsesapiwebsearchaction/supplements/find_in_page-optional-omitted-pattern.json",
                              "cases/unions/responsesapiwebsearchaction/supplements/find_in_page-nullable-null-pattern.json",
                              "cases/unions/responsesapiwebsearchaction/find_in_page.json",
                              &typed::FindInPageResponsesApiWebSearchAction::pattern) &&
                          triStateFixtures<typed::FindInPageResponsesApiWebSearchAction>(
                              detail::decodeResponsesApiWebSearchAction,
                              "cases/unions/responsesapiwebsearchaction/supplements/find_in_page-optional-omitted-url.json",
                              "cases/unions/responsesapiwebsearchaction/supplements/find_in_page-nullable-null-url.json",
                              "cases/unions/responsesapiwebsearchaction/find_in_page.json",
                              &typed::FindInPageResponsesApiWebSearchAction::url),
                      "all five Responses API web-search action optional-nullable fields exercise omitted/null/value with raw retention");

    const Json futureContentImage = fixture("cases/unions/contentitem/mutations/input_image-future-open-enum-imagedetail-detail.json");
    const auto futureContentDecoded = detail::decodeContentItem(futureContentImage);
    const auto* futureContent =
        futureContentDecoded.value ? std::get_if<typed::InputImageContentItem>(&*futureContentDecoded.value) : nullptr;
    result.expectTrue(
        futureContent && futureContent->detail.hasValue() && !futureContent->detail.value->isKnown() &&
            futureContent->raw == futureContentImage &&
            isForwardCompatible(futureContentDecoded.diagnostic, typed::DecodeIssueKind::UnknownEnumValue, "ContentItem", "$.detail") &&
            futureContent->diagnostics.size() == 1 && futureContent->diagnostics.front() == *futureContentDecoded.diagnostic,
        "future ContentItem ImageDetail stays in the typed outer alternative");

    const Json futureFunctionImage =
        fixture("cases/unions/functioncalloutputcontentitem/mutations/input_image-future-open-enum-imagedetail-detail.json");
    const auto futureFunctionDecoded = detail::decodeFunctionCallOutputContentItem(futureFunctionImage);
    const auto* futureFunction =
        futureFunctionDecoded.value ? std::get_if<typed::InputImageFunctionCallOutputContentItem>(&*futureFunctionDecoded.value) : nullptr;
    result.expectTrue(
        futureFunction && futureFunction->detail.hasValue() && !futureFunction->detail.value->isKnown() &&
            futureFunction->raw == futureFunctionImage &&
            isForwardCompatible(
                futureFunctionDecoded.diagnostic, typed::DecodeIssueKind::UnknownEnumValue, "FunctionCallOutputContentItem", "$.detail") &&
            futureFunction->diagnostics.size() == 1 && futureFunction->diagnostics.front() == *futureFunctionDecoded.diagnostic,
        "future FunctionCallOutputContentItem ImageDetail stays in the typed outer alternative");

    result.expectTrue(
        explicitUnknown<typed::UnknownAgentMessageInputContent>(detail::decodeAgentMessageInputContent,
                                                                fixture("cases/unions/agentmessageinputcontent/future-unknown.json"),
                                                                "AgentMessageInputContent"),
        "AgentMessageInputContent future discriminator selects its explicit unknown alternative");
    result.expectTrue(explicitUnknown<typed::UnknownContentItem>(
                          detail::decodeContentItem, fixture("cases/unions/contentitem/future-unknown.json"), "ContentItem"),
                      "ContentItem future discriminator selects its explicit unknown alternative");
    result.expectTrue(explicitUnknown<typed::UnknownFunctionCallOutputContentItem>(
                          detail::decodeFunctionCallOutputContentItem,
                          fixture("cases/unions/functioncalloutputcontentitem/future-unknown.json"),
                          "FunctionCallOutputContentItem"),
                      "FunctionCallOutputContentItem future discriminator selects its explicit unknown alternative");
    result.expectTrue(explicitUnknown<typed::UnknownLocalShellAction>(
                          detail::decodeLocalShellAction, fixture("cases/unions/localshellaction/future-unknown.json"), "LocalShellAction"),
                      "LocalShellAction future discriminator selects its explicit unknown alternative");
    result.expectTrue(explicitUnknown<typed::UnknownReasoningItemContent>(detail::decodeReasoningItemContent,
                                                                          fixture("cases/unions/reasoningitemcontent/future-unknown.json"),
                                                                          "ReasoningItemContent"),
                      "ReasoningItemContent future discriminator selects its explicit unknown alternative");
    result.expectTrue(explicitUnknown<typed::UnknownReasoningItemReasoningSummary>(
                          detail::decodeReasoningItemReasoningSummary,
                          fixture("cases/unions/reasoningitemreasoningsummary/future-unknown.json"),
                          "ReasoningItemReasoningSummary"),
                      "ReasoningItemReasoningSummary future discriminator selects its explicit unknown alternative");
    result.expectTrue(
        explicitUnknown<typed::UnknownResponsesApiWebSearchAction>(detail::decodeResponsesApiWebSearchAction,
                                                                   fixture("cases/unions/responsesapiwebsearchaction/future-unknown.json"),
                                                                   "ResponsesApiWebSearchAction"),
        "ResponsesApiWebSearchAction future discriminator selects its explicit unknown alternative");

    result.expectTrue(malformed(detail::decodeAgentMessageInputContent,
                                fixture("cases/unions/agentmessageinputcontent/mutations/input_text-missing-discriminator.json"),
                                "AgentMessageInputContent",
                                "$.type") &&
                          malformed(detail::decodeAgentMessageInputContent,
                                    fixture("cases/unions/agentmessageinputcontent/mutations/input_text-wrong-nested-type-text.json"),
                                    "AgentMessageInputContent",
                                    "$.text"),
                      "AgentMessageInputContent missing discriminator and wrong nested type are malformed-known");
    result.expectTrue(malformed(detail::decodeContentItem,
                                fixture("cases/unions/contentitem/mutations/input_text-missing-discriminator.json"),
                                "ContentItem",
                                "$.type") &&
                          malformed(detail::decodeContentItem,
                                    fixture("cases/unions/contentitem/mutations/input_image-wrong-nested-type-image_url.json"),
                                    "ContentItem",
                                    "$.image_url"),
                      "ContentItem missing discriminator and wrong nested type are malformed-known");
    result.expectTrue(
        malformed(detail::decodeFunctionCallOutputContentItem,
                  fixture("cases/unions/functioncalloutputcontentitem/mutations/input_text-missing-discriminator.json"),
                  "FunctionCallOutputContentItem",
                  "$.type") &&
            malformed(detail::decodeFunctionCallOutputContentItem,
                      fixture("cases/unions/functioncalloutputcontentitem/mutations/input_image-wrong-nested-type-image_url.json"),
                      "FunctionCallOutputContentItem",
                      "$.image_url"),
        "FunctionCallOutputContentItem missing discriminator and wrong nested type are malformed-known");
    result.expectTrue(malformed(detail::decodeLocalShellAction,
                                fixture("cases/unions/localshellaction/mutations/exec-missing-discriminator.json"),
                                "LocalShellAction",
                                "$.type") &&
                          malformed(detail::decodeLocalShellAction,
                                    fixture("cases/unions/localshellaction/mutations/exec-wrong-nested-type-command.json"),
                                    "LocalShellAction",
                                    "$.command"),
                      "LocalShellAction missing discriminator and wrong nested type are malformed-known");
    result.expectTrue(malformed(detail::decodeReasoningItemContent,
                                fixture("cases/unions/reasoningitemcontent/mutations/text-missing-discriminator.json"),
                                "ReasoningItemContent",
                                "$.type") &&
                          malformed(detail::decodeReasoningItemContent,
                                    fixture("cases/unions/reasoningitemcontent/mutations/reasoning_text-wrong-nested-type-text.json"),
                                    "ReasoningItemContent",
                                    "$.text"),
                      "ReasoningItemContent missing discriminator and wrong nested type are malformed-known");
    result.expectTrue(
        malformed(detail::decodeReasoningItemReasoningSummary,
                  fixture("cases/unions/reasoningitemreasoningsummary/mutations/summary_text-missing-discriminator.json"),
                  "ReasoningItemReasoningSummary",
                  "$.type") &&
            malformed(detail::decodeReasoningItemReasoningSummary,
                      fixture("cases/unions/reasoningitemreasoningsummary/mutations/summary_text-wrong-nested-type-text.json"),
                      "ReasoningItemReasoningSummary",
                      "$.text"),
        "ReasoningItemReasoningSummary missing discriminator and wrong nested type are malformed-known");
    result.expectTrue(malformed(detail::decodeResponsesApiWebSearchAction,
                                fixture("cases/unions/responsesapiwebsearchaction/mutations/search-missing-discriminator.json"),
                                "ResponsesApiWebSearchAction",
                                "$.type") &&
                          malformed(detail::decodeResponsesApiWebSearchAction,
                                    fixture("cases/unions/responsesapiwebsearchaction/mutations/search-wrong-nested-type-query.json"),
                                    "ResponsesApiWebSearchAction",
                                    "$.query"),
                      "ResponsesApiWebSearchAction missing discriminator and wrong nested type are malformed-known");

    constexpr std::array AgentWrongTypes = {
        std::pair{"cases/unions/agentmessageinputcontent/mutations/input_text-wrong-nested-type-text.json", "$.text"},
        std::pair{"cases/unions/agentmessageinputcontent/mutations/encrypted_content-wrong-nested-type-encrypted_content.json",
                  "$.encrypted_content"},
    };
    constexpr std::array ContentWrongTypes = {
        std::pair{"cases/unions/contentitem/mutations/input_text-wrong-nested-type-text.json", "$.text"},
        std::pair{"cases/unions/contentitem/mutations/input_image-wrong-nested-type-image_url.json", "$.image_url"},
        std::pair{"cases/unions/contentitem/mutations/input_image-wrong-nested-type-detail.json", "$.detail"},
        std::pair{"cases/unions/contentitem/mutations/output_text-wrong-nested-type-text.json", "$.text"},
    };
    constexpr std::array FunctionOutputWrongTypes = {
        std::pair{"cases/unions/functioncalloutputcontentitem/mutations/input_text-wrong-nested-type-text.json", "$.text"},
        std::pair{"cases/unions/functioncalloutputcontentitem/mutations/input_image-wrong-nested-type-image_url.json", "$.image_url"},
        std::pair{"cases/unions/functioncalloutputcontentitem/mutations/input_image-wrong-nested-type-detail.json", "$.detail"},
        std::pair{"cases/unions/functioncalloutputcontentitem/mutations/encrypted_content-wrong-nested-type-encrypted_content.json",
                  "$.encrypted_content"},
    };
    constexpr std::array ShellWrongTypes = {
        std::pair{"cases/unions/localshellaction/mutations/exec-wrong-nested-type-command.json", "$.command"},
        std::pair{"cases/unions/localshellaction/mutations/exec-wrong-nested-type-command-0.json", "$.command[0]"},
        std::pair{"cases/unions/localshellaction/mutations/exec-wrong-nested-type-env.json", "$.env"},
        std::pair{"cases/unions/localshellaction/mutations/exec-wrong-nested-type-env-synthetickey.json", "$.env"},
        std::pair{"cases/unions/localshellaction/mutations/exec-wrong-nested-type-timeout_ms.json", "$.timeout_ms"},
        std::pair{"cases/unions/localshellaction/mutations/exec-wrong-nested-type-user.json", "$.user"},
        std::pair{"cases/unions/localshellaction/mutations/exec-wrong-nested-type-working_directory.json", "$.working_directory"},
    };
    constexpr std::array ReasoningWrongTypes = {
        std::pair{"cases/unions/reasoningitemcontent/mutations/reasoning_text-wrong-nested-type-text.json", "$.text"},
        std::pair{"cases/unions/reasoningitemcontent/mutations/text-wrong-nested-type-text.json", "$.text"},
    };
    constexpr std::array SummaryWrongTypes = {
        std::pair{"cases/unions/reasoningitemreasoningsummary/mutations/summary_text-wrong-nested-type-text.json", "$.text"},
    };
    constexpr std::array ResponsesWebWrongTypes = {
        std::pair{"cases/unions/responsesapiwebsearchaction/mutations/search-wrong-nested-type-queries.json", "$.queries"},
        std::pair{"cases/unions/responsesapiwebsearchaction/mutations/search-wrong-nested-type-query.json", "$.query"},
        std::pair{"cases/unions/responsesapiwebsearchaction/mutations/open_page-wrong-nested-type-url.json", "$.url"},
        std::pair{"cases/unions/responsesapiwebsearchaction/mutations/find_in_page-wrong-nested-type-pattern.json", "$.pattern"},
        std::pair{"cases/unions/responsesapiwebsearchaction/mutations/find_in_page-wrong-nested-type-url.json", "$.url"},
    };
    result.expectTrue(
        allMalformed(detail::decodeAgentMessageInputContent, "AgentMessageInputContent", AgentWrongTypes) &&
            allMalformed(detail::decodeContentItem, "ContentItem", ContentWrongTypes) &&
            allMalformed(detail::decodeFunctionCallOutputContentItem, "FunctionCallOutputContentItem", FunctionOutputWrongTypes) &&
            allMalformed(detail::decodeLocalShellAction, "LocalShellAction", ShellWrongTypes) &&
            allMalformed(detail::decodeReasoningItemContent, "ReasoningItemContent", ReasoningWrongTypes) &&
            allMalformed(detail::decodeReasoningItemReasoningSummary, "ReasoningItemReasoningSummary", SummaryWrongTypes) &&
            allMalformed(detail::decodeResponsesApiWebSearchAction, "ResponsesApiWebSearchAction", ResponsesWebWrongTypes),
        "all 25 represented nested fields and container values reject their independently generated wrong-type mutations at exact paths");

    constexpr std::array AgentMissingRequired = {
        std::pair{"cases/unions/agentmessageinputcontent/mutations/input_text-missing-required-text.json", "$.text"},
        std::pair{"cases/unions/agentmessageinputcontent/mutations/encrypted_content-missing-required-encrypted_content.json",
                  "$.encrypted_content"},
    };
    constexpr std::array ContentMissingRequired = {
        std::pair{"cases/unions/contentitem/mutations/input_text-missing-required-text.json", "$.text"},
        std::pair{"cases/unions/contentitem/mutations/input_image-missing-required-image_url.json", "$.image_url"},
        std::pair{"cases/unions/contentitem/mutations/output_text-missing-required-text.json", "$.text"},
    };
    constexpr std::array FunctionOutputMissingRequired = {
        std::pair{"cases/unions/functioncalloutputcontentitem/mutations/input_text-missing-required-text.json", "$.text"},
        std::pair{"cases/unions/functioncalloutputcontentitem/mutations/input_image-missing-required-image_url.json", "$.image_url"},
        std::pair{"cases/unions/functioncalloutputcontentitem/mutations/encrypted_content-missing-required-encrypted_content.json",
                  "$.encrypted_content"},
    };
    constexpr std::array ShellMissingRequired = {
        std::pair{"cases/unions/localshellaction/mutations/exec-missing-required-command.json", "$.command"},
    };
    constexpr std::array ReasoningMissingRequired = {
        std::pair{"cases/unions/reasoningitemcontent/mutations/reasoning_text-missing-required-text.json", "$.text"},
        std::pair{"cases/unions/reasoningitemcontent/mutations/text-missing-required-text.json", "$.text"},
    };
    constexpr std::array SummaryMissingRequired = {
        std::pair{"cases/unions/reasoningitemreasoningsummary/mutations/summary_text-missing-required-text.json", "$.text"},
    };
    result.expectTrue(
        allMalformed(detail::decodeAgentMessageInputContent, "AgentMessageInputContent", AgentMissingRequired) &&
            allMalformed(detail::decodeContentItem, "ContentItem", ContentMissingRequired) &&
            allMalformed(detail::decodeFunctionCallOutputContentItem, "FunctionCallOutputContentItem", FunctionOutputMissingRequired) &&
            allMalformed(detail::decodeLocalShellAction, "LocalShellAction", ShellMissingRequired) &&
            allMalformed(detail::decodeReasoningItemContent, "ReasoningItemContent", ReasoningMissingRequired) &&
            allMalformed(detail::decodeReasoningItemReasoningSummary, "ReasoningItemReasoningSummary", SummaryMissingRequired),
        "all 12 required nested value fields reject independently generated missing-field mutations at exact paths");

    constexpr std::string_view Sensitive = "secret-token-that-must-not-enter-diagnostics";
    const Json sensitiveShell = {
        {"type", "exec"},
        {"command", Json::array({Sensitive})},
        {"env", Json::array()},
    };
    const auto sensitiveDecoded = detail::decodeLocalShellAction(sensitiveShell);
    result.expectTrue(!sensitiveDecoded.value && sensitiveDecoded.diagnostic &&
                          sensitiveDecoded.diagnostic->message.find(Sensitive) == std::string::npos &&
                          sensitiveDecoded.diagnostic->surface.find(Sensitive) == std::string::npos &&
                          sensitiveDecoded.diagnostic->fieldPath.find(Sensitive) == std::string::npos &&
                          sensitiveShell.dump().find(Sensitive) != std::string::npos,
                      "nested-union diagnostics retain identity/path but omit sensitive payload values");

    struct ExpectedDescriptor {
        std::string_view domain;
        std::string_view name;
        detail::ConversationUnionTarget target;
    };
    constexpr std::array ExpectedDescriptors = {
        ExpectedDescriptor{
            "AgentMessageInputContent", "encrypted_content", detail::ConversationUnionTarget::AgentMessageInputContentEncryptedContent},
        ExpectedDescriptor{"AgentMessageInputContent", "input_text", detail::ConversationUnionTarget::AgentMessageInputContentInputText},
        ExpectedDescriptor{"ContentItem", "input_image", detail::ConversationUnionTarget::ContentItemInputImage},
        ExpectedDescriptor{"ContentItem", "input_text", detail::ConversationUnionTarget::ContentItemInputText},
        ExpectedDescriptor{"ContentItem", "output_text", detail::ConversationUnionTarget::ContentItemOutputText},
        ExpectedDescriptor{"FunctionCallOutputContentItem",
                           "encrypted_content",
                           detail::ConversationUnionTarget::FunctionCallOutputContentItemEncryptedContent},
        ExpectedDescriptor{
            "FunctionCallOutputContentItem", "input_image", detail::ConversationUnionTarget::FunctionCallOutputContentItemInputImage},
        ExpectedDescriptor{
            "FunctionCallOutputContentItem", "input_text", detail::ConversationUnionTarget::FunctionCallOutputContentItemInputText},
        ExpectedDescriptor{"LocalShellAction", "exec", detail::ConversationUnionTarget::LocalShellActionExec},
        ExpectedDescriptor{"ReasoningItemContent", "reasoning_text", detail::ConversationUnionTarget::ReasoningItemContentReasoningText},
        ExpectedDescriptor{"ReasoningItemContent", "text", detail::ConversationUnionTarget::ReasoningItemContentText},
        ExpectedDescriptor{
            "ReasoningItemReasoningSummary", "summary_text", detail::ConversationUnionTarget::ReasoningItemReasoningSummarySummaryText},
        ExpectedDescriptor{
            "ResponsesApiWebSearchAction", "find_in_page", detail::ConversationUnionTarget::ResponsesApiWebSearchActionFindInPage},
        ExpectedDescriptor{
            "ResponsesApiWebSearchAction", "open_page", detail::ConversationUnionTarget::ResponsesApiWebSearchActionOpenPage},
        ExpectedDescriptor{"ResponsesApiWebSearchAction", "other", detail::ConversationUnionTarget::ResponsesApiWebSearchActionOther},
        ExpectedDescriptor{"ResponsesApiWebSearchAction", "search", detail::ConversationUnionTarget::ResponsesApiWebSearchActionSearch},
    };
    bool exactRegistryTargets = true;
    const std::span<const detail::ConversationUnionCodecDescriptor> descriptors = detail::conversationUnionCodecDescriptors();
    for (const ExpectedDescriptor& expected : ExpectedDescriptors) {
        const detail::ProtocolSurfaceEntry* row =
            detail::findSurface(detail::SurfaceCategory::TaggedUnionDiscriminator, expected.domain, "type", expected.name);
        if (row == nullptr) {
            exactRegistryTargets = false;
            continue;
        }
        const auto* target = std::get_if<detail::ConversationUnionTarget>(&row->runtimeTarget);
        const auto descriptor =
            std::find_if(descriptors.begin(), descriptors.end(), [&](const detail::ConversationUnionCodecDescriptor& candidate) {
                return candidate.key == row->key && candidate.target == expected.target;
            });
        exactRegistryTargets =
            exactRegistryTargets && row->runtimeDisposition == detail::RuntimeDisposition::Typed &&
            row->typedImplementation == detail::TypedImplementationStatus::Implemented && target && *target == expected.target &&
            descriptor != descriptors.end() && descriptor->direction == detail::ConversationUnionCodecDirection::DecodeOnly &&
            descriptor->shape == detail::ConversationUnionCodecShape::InternallyTaggedObject && &detail::entryFor(expected.target) == row;
    }
    result.expectTrue(exactRegistryTargets, "all 16 nested alternatives dispatch through exact canonical registry targets");

    return result.processResult();
}
