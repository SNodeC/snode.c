/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef TESTS_COMPONENT_CODEX_CODEXTYPEDSURFACEBASELINE_H
#define TESTS_COMPONENT_CODEX_CODEXTYPEDSURFACEBASELINE_H

#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"

#include <array>
#include <string_view>

namespace tests::component::codex {

    struct TypedSurfaceIdentity {
        ai::openai::codex::detail::SurfaceCategory category;
        std::string_view domain;
        std::string_view field;
        std::string_view name;
    };

    // This is an intentionally reviewed, non-generated no-regression floor.
    // New typed identities may be added without changing it. Removing or
    // replacing an identity below requires an explicit reviewed baseline edit.
    inline constexpr std::array<TypedSurfaceIdentity, 34> TypedSurfaceBaseline{{
        {ai::openai::codex::detail::SurfaceCategory::ClientNotification, "ClientNotification", "method", "initialized"},

        {ai::openai::codex::detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "initialize"},
        {ai::openai::codex::detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "thread/list"},
        {ai::openai::codex::detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "thread/read"},
        {ai::openai::codex::detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "thread/resume"},
        {ai::openai::codex::detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "thread/start"},
        {ai::openai::codex::detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "turn/interrupt"},
        {ai::openai::codex::detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "turn/start"},

        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "error"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "item/agentMessage/delta"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification,
         "ServerNotification",
         "method",
         "item/commandExecution/outputDelta"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "item/completed"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "item/fileChange/patchUpdated"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "item/reasoning/summaryTextDelta"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "item/reasoning/textDelta"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "item/started"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "model/rerouted"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "thread/started"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "thread/status/changed"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "thread/tokenUsage/updated"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "turn/completed"},
        {ai::openai::codex::detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "turn/started"},

        {ai::openai::codex::detail::SurfaceCategory::ServerRequest, "ServerRequest", "method", "account/chatgptAuthTokens/refresh"},
        {ai::openai::codex::detail::SurfaceCategory::ServerRequest, "ServerRequest", "method", "item/commandExecution/requestApproval"},
        {ai::openai::codex::detail::SurfaceCategory::ServerRequest, "ServerRequest", "method", "item/fileChange/requestApproval"},
        {ai::openai::codex::detail::SurfaceCategory::ServerRequest, "ServerRequest", "method", "item/tool/requestUserInput"},

        {ai::openai::codex::detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "agentMessage"},
        {ai::openai::codex::detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "commandExecution"},
        {ai::openai::codex::detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "dynamicToolCall"},
        {ai::openai::codex::detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "fileChange"},
        {ai::openai::codex::detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "mcpToolCall"},
        {ai::openai::codex::detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "reasoning"},
        {ai::openai::codex::detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "userMessage"},
        {ai::openai::codex::detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "webSearch"},
    }};

} // namespace tests::component::codex

#endif // TESTS_COMPONENT_CODEX_CODEXTYPEDSURFACEBASELINE_H
