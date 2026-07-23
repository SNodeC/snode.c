/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef TESTS_COMPONENT_CODEX_CODEXA11B2TYPEDSURFACEBASELINE_H
#define TESTS_COMPONENT_CODEX_CODEXA11B2TYPEDSURFACEBASELINE_H

#include "component/codex/CodexTypedSurfaceBaseline.h"

#include <array>

namespace tests::component::codex {

    // Reviewed A1.1 Commit 2 addition. This is an exact no-regression floor,
    // deliberately separate from both A1.0 baselines.
    inline constexpr std::array<TypedSurfaceIdentity, 26> CodexA11B2TypedSurfaceBaseline{{
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "granular"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "never"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "on-request"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "untrusted"},

        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "CommandAction", "type", "listFiles"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "CommandAction", "type", "read"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "CommandAction", "type", "search"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "CommandAction", "type", "unknown"},

        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "DynamicToolCallOutputContentItem", "type", "inputImage"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "DynamicToolCallOutputContentItem", "type", "inputText"},

        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "PatchChangeKind", "type", "add"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "PatchChangeKind", "type", "delete"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "PatchChangeKind", "type", "update"},

        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "SandboxPolicy", "type", "dangerFullAccess"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "SandboxPolicy", "type", "externalSandbox"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "SandboxPolicy", "type", "readOnly"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "SandboxPolicy", "type", "workspaceWrite"},

        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "image"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "localImage"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "mention"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "skill"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "text"},

        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "WebSearchAction", "type", "findInPage"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "WebSearchAction", "type", "openPage"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "WebSearchAction", "type", "other"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "WebSearchAction", "type", "search"},
    }};

} // namespace tests::component::codex

#endif // TESTS_COMPONENT_CODEX_CODEXA11B2TYPEDSURFACEBASELINE_H
