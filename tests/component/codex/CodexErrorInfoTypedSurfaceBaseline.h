/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef TESTS_COMPONENT_CODEX_CODEXERRORINFOTYPEDSURFACEBASELINE_H
#define TESTS_COMPONENT_CODEX_CODEXERRORINFOTYPEDSURFACEBASELINE_H

#include "component/codex/CodexTypedSurfaceBaseline.h"

#include <array>

namespace tests::component::codex {

    // Reviewed A1.0 addition to the original exact 34-identity runtime floor.
    inline constexpr std::array<TypedSurfaceIdentity, 16> CodexErrorInfoTypedSurfaceBaseline{{
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "activeTurnNotSteerable"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "CodexErrorInfo", "$variant", "badRequest"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "contextWindowExceeded"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "CodexErrorInfo", "$variant", "cyberPolicy"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "httpConnectionFailed"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "internalServerError"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "CodexErrorInfo", "$variant", "other"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "responseStreamConnectionFailed"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "responseStreamDisconnected"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "responseTooManyFailedAttempts"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "CodexErrorInfo", "$variant", "sandboxError"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "serverOverloaded"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "sessionBudgetExceeded"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "threadRollbackFailed"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator, "CodexErrorInfo", "$variant", "unauthorized"},
        {ai::openai::codex::detail::SurfaceCategory::TaggedUnionDiscriminator,
         "CodexErrorInfo",
         "$variant",
         "usageLimitExceeded"},
    }};

} // namespace tests::component::codex

#endif // TESTS_COMPONENT_CODEX_CODEXERRORINFOTYPEDSURFACEBASELINE_H
