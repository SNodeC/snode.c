/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_TYPES_H
#define AI_OPENAI_CODEX_TYPED_TYPES_H

#include "ai/openai/codex/Protocol.h"

#include <compare>
#include <nlohmann/json.hpp>
#include <string>

namespace ai::openai::codex::typed {

    struct ThreadId {
        std::string value;

        auto operator<=>(const ThreadId&) const = default;
    };

    struct TurnId {
        std::string value;

        auto operator<=>(const TurnId&) const = default;
    };

    struct ItemId {
        std::string value;

        auto operator<=>(const ItemId&) const = default;
    };

    struct ModelId {
        std::string value;

        auto operator<=>(const ModelId&) const = default;
    };

    struct ReasoningEffort {
        std::string value;

        static ReasoningEffort minimal();
        static ReasoningEffort low();
        static ReasoningEffort medium();
        static ReasoningEffort high();
        static ReasoningEffort xhigh();

        auto operator<=>(const ReasoningEffort&) const = default;
    };

    struct ApprovalPolicy {
        std::string value;

        static ApprovalPolicy untrusted();
        static ApprovalPolicy onRequest();
        static ApprovalPolicy never();

        auto operator<=>(const ApprovalPolicy&) const = default;
    };

    struct SandboxMode {
        std::string value;

        static SandboxMode readOnly();
        static SandboxMode workspaceWrite();
        static SandboxMode dangerFullAccess();

        auto operator<=>(const SandboxMode&) const = default;
    };

    struct ImageDetail {
        std::string value;

        static ImageDetail automatic();
        static ImageDetail low();
        static ImageDetail high();
        static ImageDetail original();

        auto operator<=>(const ImageDetail&) const = default;
    };

    struct NetworkAccess {
        std::string value;

        static NetworkAccess restricted();
        static NetworkAccess enabled();

        auto operator<=>(const NetworkAccess&) const = default;
    };

    struct ThreadStatus {
        std::string value;
        Json raw = Json::object();
    };

    struct TurnStatus {
        std::string value;

        static TurnStatus completed();
        static TurnStatus interrupted();
        static TurnStatus failed();
        static TurnStatus inProgress();

        auto operator<=>(const TurnStatus&) const = default;
    };

    struct TurnItemsView {
        std::string value;

        static TurnItemsView notLoaded();
        static TurnItemsView summary();
        static TurnItemsView full();

        auto operator<=>(const TurnItemsView&) const = default;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_TYPES_H
