/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/typed/Configuration.h"

namespace ai::openai::codex::typed {

    AutoCompactTokenLimitScope AutoCompactTokenLimitScope::total() {
        return {"total"};
    }

    AutoCompactTokenLimitScope AutoCompactTokenLimitScope::bodyAfterPrefix() {
        return {"body_after_prefix"};
    }

    bool AutoCompactTokenLimitScope::isKnown() const noexcept {
        return value == "total" || value == "body_after_prefix";
    }

    ForcedLoginMethod ForcedLoginMethod::chatgpt() {
        return {"chatgpt"};
    }

    ForcedLoginMethod ForcedLoginMethod::api() {
        return {"api"};
    }

    bool ForcedLoginMethod::isKnown() const noexcept {
        return value == "chatgpt" || value == "api";
    }

    ResidencyRequirement ResidencyRequirement::us() {
        return {"us"};
    }

    bool ResidencyRequirement::isKnown() const noexcept {
        return value == "us";
    }

    Verbosity Verbosity::low() {
        return {"low"};
    }

    Verbosity Verbosity::medium() {
        return {"medium"};
    }

    Verbosity Verbosity::high() {
        return {"high"};
    }

    bool Verbosity::isKnown() const noexcept {
        return value == "low" || value == "medium" || value == "high";
    }

    WebSearchContextSize WebSearchContextSize::low() {
        return {"low"};
    }

    WebSearchContextSize WebSearchContextSize::medium() {
        return {"medium"};
    }

    WebSearchContextSize WebSearchContextSize::high() {
        return {"high"};
    }

    bool WebSearchContextSize::isKnown() const noexcept {
        return value == "low" || value == "medium" || value == "high";
    }

    WebSearchMode WebSearchMode::disabled() {
        return {"disabled"};
    }

    WebSearchMode WebSearchMode::cached() {
        return {"cached"};
    }

    WebSearchMode WebSearchMode::indexed() {
        return {"indexed"};
    }

    WebSearchMode WebSearchMode::live() {
        return {"live"};
    }

    bool WebSearchMode::isKnown() const noexcept {
        return value == "disabled" || value == "cached" || value == "indexed" || value == "live";
    }

    WindowsSandboxSetupMode WindowsSandboxSetupMode::elevated() {
        return {"elevated"};
    }

    WindowsSandboxSetupMode WindowsSandboxSetupMode::unelevated() {
        return {"unelevated"};
    }

    bool WindowsSandboxSetupMode::isKnown() const noexcept {
        return value == "elevated" || value == "unelevated";
    }

} // namespace ai::openai::codex::typed
