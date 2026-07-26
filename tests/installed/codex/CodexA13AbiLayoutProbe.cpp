/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Client.h"
#include "ai/openai/codex/typed/Events.h"
#include "ai/openai/codex/typed/ServerRequests.h"

#include <cstddef>
#include <iostream>

int main() {
    using ai::openai::codex::AppServerClient;
    namespace typed = ai::openai::codex::typed;

    const auto printSize = [](const char* name, std::size_t size) {
        std::cout << name << '=' << size << '\n';
    };
    printSize("AppServerClient", sizeof(AppServerClient));
    printSize("Client", sizeof(typed::Client));
    printSize("Event", sizeof(typed::Event));
    printSize("CanonicalServerNotification", sizeof(typed::CanonicalServerNotification));
    printSize("TypedServerRequest", sizeof(typed::TypedServerRequest));
    printSize("CommandApprovalRequest", sizeof(typed::CommandApprovalRequest));
    printSize("FileChangeApprovalRequest", sizeof(typed::FileChangeApprovalRequest));
}
