/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/typed/Models.h"

namespace ai::openai::codex::typed {

    InputModality InputModality::text() {
        return {"text"};
    }

    InputModality InputModality::image() {
        return {"image"};
    }

    bool InputModality::isKnown() const noexcept {
        return value == "text" || value == "image";
    }

    ModelRerouteReason ModelRerouteReason::highRiskCyberActivity() {
        return {"highRiskCyberActivity"};
    }

    bool ModelRerouteReason::isKnown() const noexcept {
        return value == "highRiskCyberActivity";
    }

    ModelVerification ModelVerification::trustedAccessForCyber() {
        return {"trustedAccessForCyber"};
    }

    bool ModelVerification::isKnown() const noexcept {
        return value == "trustedAccessForCyber";
    }

} // namespace ai::openai::codex::typed
