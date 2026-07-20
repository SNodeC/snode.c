/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CLIENT_CONFIGURATION_H
#define APPS_CODEX_BACKEND_CLIENT_CONFIGURATION_H

#include <string>

namespace apps::codex_backend_client {

    // Matches the reference backend's default exactly. The value is installed
    // as a ConfigSocketClient default; SNode.C CLI/config overrides remain
    // authoritative.
    [[nodiscard]] std::string defaultSocketPath();

} // namespace apps::codex_backend_client

#endif // APPS_CODEX_BACKEND_CLIENT_CONFIGURATION_H
