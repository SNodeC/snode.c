/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include <ai/openai/codex/stdio/Client.h>
#include <ai/openai/codex/typed/Client.h>

int main() {
    ai::openai::codex::stdio::Client client;
    const ai::openai::codex::AppServerClient& constClient = client;

    const bool sameObjects = &client.threads() == &client.typed().threads() &&
                             &client.turns() == &client.typed().turns() &&
                             &client.events() == &client.typed().events() &&
                             &client.requests() == &client.typed().requests() &&
                             &constClient.threads() == &constClient.typed().threads() &&
                             &constClient.turns() == &constClient.typed().turns() &&
                             &constClient.events() == &constClient.typed().events() &&
                             &constClient.requests() == &constClient.typed().requests();

    return sameObjects ? 0 : 1;
}
