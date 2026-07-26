/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_PERMISSIONPROFILES_H
#define AI_OPENAI_CODEX_TYPED_PERMISSIONPROFILES_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Types.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ai::openai::codex::typed {

    struct PermissionProfileListParams {
        OptionalNullable<std::string> cursor;
        OptionalNullable<std::string> cwd;
        OptionalNullable<std::uint32_t> limit;

        bool operator==(const PermissionProfileListParams&) const = default;
    };

    struct PermissionProfileSummary {
        bool allowed = false;
        OptionalNullable<std::string> description;
        std::string id;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const PermissionProfileSummary&) const = default;
    };

    struct PermissionProfileListResponse {
        std::vector<PermissionProfileSummary> data;
        OptionalNullable<std::string> nextCursor;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const PermissionProfileListResponse&) const = default;
    };

    class PermissionProfiles {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using ListResultHandler = std::function<void(const OperationResult<PermissionProfileListResponse>&)>;

        Submission list(PermissionProfileListParams params, ListResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit PermissionProfiles(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_PERMISSIONPROFILES_H
