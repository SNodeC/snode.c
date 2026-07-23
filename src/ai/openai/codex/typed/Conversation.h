/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_CONVERSATION_H
#define AI_OPENAI_CODEX_TYPED_CONVERSATION_H

#include "ai/openai/codex/typed/Types.h"

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct GranularApprovalOptions {
        bool mcpElicitations = false;
        std::optional<bool> requestPermissions;
        bool rules = false;
        bool sandboxApproval = false;
        std::optional<bool> skillApproval;

        [[nodiscard]] bool requestPermissionsOrDefault() const noexcept {
            return requestPermissions.value_or(false);
        }

        [[nodiscard]] bool skillApprovalOrDefault() const noexcept {
            return skillApproval.value_or(false);
        }

        auto operator<=>(const GranularApprovalOptions&) const = default;
    };

    struct GranularAskForApproval {
        GranularApprovalOptions granular;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const GranularAskForApproval&) const = default;
    };

    struct UnknownAskForApproval {
        std::optional<std::string> discriminator;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownAskForApproval&) const = default;
    };

    // The three stable scalar alternatives share the open ApprovalPolicy value
    // type; the object branch and a future branch remain directionally explicit.
    using AskForApproval = std::variant<ApprovalPolicy, GranularAskForApproval, UnknownAskForApproval>;

    struct ReadCommandAction {
        std::string command;
        std::string name;
        AbsolutePathBuf path;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ReadCommandAction&) const = default;
    };

    struct ListFilesCommandAction {
        std::string command;
        OptionalNullable<std::string> path;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ListFilesCommandAction&) const = default;
    };

    struct SearchCommandAction {
        std::string command;
        OptionalNullable<std::string> path;
        OptionalNullable<std::string> query;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SearchCommandAction&) const = default;
    };

    // "unknown" is a protocol-defined known discriminator, not the
    // forward-compatibility alternative below.
    struct UnknownCommandAction {
        std::string command;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const UnknownCommandAction&) const = default;
    };

    struct UnrecognizedCommandAction {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnrecognizedCommandAction&) const = default;
    };

    using CommandAction =
        std::variant<ReadCommandAction, ListFilesCommandAction, SearchCommandAction, UnknownCommandAction, UnrecognizedCommandAction>;

    struct InputTextDynamicToolCallOutputContentItem {
        std::string text;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const InputTextDynamicToolCallOutputContentItem&) const = default;
    };

    struct InputImageDynamicToolCallOutputContentItem {
        std::string imageUrl;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const InputImageDynamicToolCallOutputContentItem&) const = default;
    };

    struct UnknownDynamicToolCallOutputContentItem {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownDynamicToolCallOutputContentItem&) const = default;
    };

    using DynamicToolCallOutputContentItem = std::variant<InputTextDynamicToolCallOutputContentItem,
                                                          InputImageDynamicToolCallOutputContentItem,
                                                          UnknownDynamicToolCallOutputContentItem>;

    struct AddPatchChangeKind {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AddPatchChangeKind&) const = default;
    };

    struct DeletePatchChangeKind {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const DeletePatchChangeKind&) const = default;
    };

    struct UpdatePatchChangeKind {
        OptionalNullable<std::string> movePath;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const UpdatePatchChangeKind&) const = default;
    };

    struct UnknownPatchChangeKind {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownPatchChangeKind&) const = default;
    };

    using PatchChangeKind = std::variant<AddPatchChangeKind, DeletePatchChangeKind, UpdatePatchChangeKind, UnknownPatchChangeKind>;

    struct DangerFullAccessSandboxPolicy {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const DangerFullAccessSandboxPolicy&) const = default;
    };

    struct ReadOnlySandboxPolicy {
        std::optional<bool> networkAccess;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        [[nodiscard]] bool networkAccessOrDefault() const noexcept {
            return networkAccess.value_or(false);
        }

        bool operator==(const ReadOnlySandboxPolicy&) const = default;
    };

    struct ExternalSandboxSandboxPolicy {
        std::optional<NetworkAccess> networkAccess;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        [[nodiscard]] NetworkAccess networkAccessOrDefault() const {
            return networkAccess.value_or(NetworkAccess::restricted());
        }

        bool operator==(const ExternalSandboxSandboxPolicy&) const = default;
    };

    // Transitional source compatibility for the shorter A0/A1.0 name.
    using ExternalSandboxPolicy = ExternalSandboxSandboxPolicy;

    struct WorkspaceWriteSandboxPolicy {
        std::optional<std::vector<AbsolutePathBuf>> writableRoots;
        std::optional<bool> networkAccess;
        std::optional<bool> excludeTmpdirEnvVar;
        std::optional<bool> excludeSlashTmp;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        [[nodiscard]] bool networkAccessOrDefault() const noexcept {
            return networkAccess.value_or(false);
        }

        [[nodiscard]] bool excludeTmpdirEnvVarOrDefault() const noexcept {
            return excludeTmpdirEnvVar.value_or(false);
        }

        [[nodiscard]] bool excludeSlashTmpOrDefault() const noexcept {
            return excludeSlashTmp.value_or(false);
        }

        [[deprecated("construct WorkspaceWriteSandboxPolicy with "
                     "optional<vector<AbsolutePathBuf>>")]] [[nodiscard]] static WorkspaceWriteSandboxPolicy
        fromLegacy(std::vector<std::string> roots,
                   std::optional<bool> allowNetwork = std::nullopt,
                   std::optional<bool> excludeTmpdir = std::nullopt,
                   std::optional<bool> excludeSlash = std::nullopt) {
            WorkspaceWriteSandboxPolicy result;
            if (!roots.empty()) {
                std::vector<AbsolutePathBuf> typedRoots;
                typedRoots.reserve(roots.size());
                for (std::string& root : roots) {
                    typedRoots.push_back(AbsolutePathBuf{std::move(root)});
                }
                result.writableRoots = std::move(typedRoots);
            }
            result.networkAccess = allowNetwork;
            result.excludeTmpdirEnvVar = excludeTmpdir;
            result.excludeSlashTmp = excludeSlash;
            return result;
        }

        bool operator==(const WorkspaceWriteSandboxPolicy&) const = default;
    };

    struct UnknownSandboxPolicy {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownSandboxPolicy&) const = default;
    };

    using SandboxPolicy = std::variant<DangerFullAccessSandboxPolicy,
                                       ReadOnlySandboxPolicy,
                                       ExternalSandboxSandboxPolicy,
                                       WorkspaceWriteSandboxPolicy,
                                       UnknownSandboxPolicy>;

    struct TextUserInput {
        std::string text;
        // Preserve the A0/A1.0 TextInput{"..."} wire behavior, which emitted
        // the protocol default empty array. Callers can still assign
        // std::nullopt to exercise an omitted field explicitly.
        std::optional<std::vector<TextElement>> textElements = std::vector<TextElement>{};
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const TextUserInput&) const = default;
    };

    struct ImageUserInput {
        std::string url;
        OptionalNullable<ImageDetail> detail;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ImageUserInput&) const = default;
    };

    struct LocalImageUserInput {
        std::string path;
        OptionalNullable<ImageDetail> detail;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const LocalImageUserInput&) const = default;
    };

    struct SkillUserInput {
        std::string name;
        std::string path;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SkillUserInput&) const = default;
    };

    struct MentionUserInput {
        std::string name;
        std::string path;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const MentionUserInput&) const = default;
    };

    struct UnknownUserInput {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownUserInput&) const = default;
    };

    using UserInput = std::variant<TextUserInput, ImageUserInput, LocalImageUserInput, SkillUserInput, MentionUserInput, UnknownUserInput>;

    // Transitional source compatibility for the A0/A1.0 input vocabulary.
    using TextInput = TextUserInput;
    using ImageUrlInput = ImageUserInput;
    using LocalImageInput = LocalImageUserInput;
    using SkillInput = SkillUserInput;
    using MentionInput = MentionUserInput;
    using UnknownTurnInput = UnknownUserInput;
    using TurnInput = UserInput;

    struct SearchWebSearchAction {
        OptionalNullable<std::vector<std::string>> queries;
        OptionalNullable<std::string> query;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SearchWebSearchAction&) const = default;
    };

    struct OpenPageWebSearchAction {
        OptionalNullable<std::string> url;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const OpenPageWebSearchAction&) const = default;
    };

    struct FindInPageWebSearchAction {
        OptionalNullable<std::string> pattern;
        OptionalNullable<std::string> url;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FindInPageWebSearchAction&) const = default;
    };

    struct OtherWebSearchAction {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const OtherWebSearchAction&) const = default;
    };

    struct UnknownWebSearchAction {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownWebSearchAction&) const = default;
    };

    using WebSearchAction = std::
        variant<SearchWebSearchAction, OpenPageWebSearchAction, FindInPageWebSearchAction, OtherWebSearchAction, UnknownWebSearchAction>;

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_CONVERSATION_H
