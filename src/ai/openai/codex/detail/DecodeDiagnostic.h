/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_DECODEDIAGNOSTIC_H
#define AI_OPENAI_CODEX_DETAIL_DECODEDIAGNOSTIC_H

#include "ai/openai/codex/typed/Types.h"

#include <string>
#include <utility>

namespace ai::openai::codex::detail {

    inline typed::DecodeDiagnostic makeDecodeDiagnostic(typed::DecodeIssueKind kind,
                                                        typed::DecodeIssueSeverity severity,
                                                        std::string surface,
                                                        std::string fieldPath) {
        const char* message = "known protocol payload did not match its typed contract";
        switch (kind) {
            case typed::DecodeIssueKind::UnknownMethod:
                message = "unrecognized protocol method was retained as raw JSON";
                break;
            case typed::DecodeIssueKind::UnknownDiscriminator:
                message = "unrecognized protocol discriminator was retained as raw JSON";
                break;
            case typed::DecodeIssueKind::UnknownEnumValue:
                message = "unrecognized protocol enum value was retained";
                break;
            case typed::DecodeIssueKind::MalformedKnownPayload:
                break;
        }
        return {kind, severity, std::move(surface), std::move(fieldPath), message};
    }

    inline typed::DecodeDiagnostic unknownMethodDiagnostic(std::string surface, std::string fieldPath = "$.method") {
        return makeDecodeDiagnostic(typed::DecodeIssueKind::UnknownMethod,
                                    typed::DecodeIssueSeverity::ForwardCompatibility,
                                    std::move(surface),
                                    std::move(fieldPath));
    }

    inline typed::DecodeDiagnostic unknownDiscriminatorDiagnostic(std::string surface, std::string fieldPath) {
        return makeDecodeDiagnostic(typed::DecodeIssueKind::UnknownDiscriminator,
                                    typed::DecodeIssueSeverity::ForwardCompatibility,
                                    std::move(surface),
                                    std::move(fieldPath));
    }

    inline typed::DecodeDiagnostic unknownEnumDiagnostic(std::string surface, std::string fieldPath) {
        return makeDecodeDiagnostic(typed::DecodeIssueKind::UnknownEnumValue,
                                    typed::DecodeIssueSeverity::ForwardCompatibility,
                                    std::move(surface),
                                    std::move(fieldPath));
    }

    inline typed::DecodeDiagnostic malformedKnownDiagnostic(std::string surface, std::string fieldPath) {
        return makeDecodeDiagnostic(typed::DecodeIssueKind::MalformedKnownPayload,
                                    typed::DecodeIssueSeverity::ProtocolWarning,
                                    std::move(surface),
                                    std::move(fieldPath));
    }

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_DECODEDIAGNOSTIC_H
