/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_CLIENT_H
#define AI_OPENAI_CODEX_TYPED_CLIENT_H

#include "ai/openai/codex/typed/Accounts.h"       // IWYU pragma: export
#include "ai/openai/codex/typed/Configuration.h"  // IWYU pragma: export
#include "ai/openai/codex/typed/Events.h"         // IWYU pragma: export
#include "ai/openai/codex/typed/Models.h"         // IWYU pragma: export
#include "ai/openai/codex/typed/ServerRequests.h" // IWYU pragma: export
#include "ai/openai/codex/typed/Threads.h"        // IWYU pragma: export
#include "ai/openai/codex/typed/Turns.h"          // IWYU pragma: export

#include <memory>

namespace ai::openai::codex {
    class AppServerClient;
}

namespace ai::openai::codex::typed {

    class Client {
    public:
        Client(const Client&) = delete;
        Client(Client&&) = delete;

        Client& operator=(const Client&) = delete;
        Client& operator=(Client&&) = delete;

        ~Client();

        Accounts& accounts() noexcept;
        const Accounts& accounts() const noexcept;

        Configuration& configuration() noexcept;
        const Configuration& configuration() const noexcept;

        Models& models() noexcept;
        const Models& models() const noexcept;

        Threads& threads() noexcept;
        const Threads& threads() const noexcept;

        Turns& turns() noexcept;
        const Turns& turns() const noexcept;

        Events& events() noexcept;
        const Events& events() const noexcept;

        Requests& requests() noexcept;
        const Requests& requests() const noexcept;

    private:
        friend class ::ai::openai::codex::AppServerClient;

        Client(std::unique_ptr<Accounts> accounts,
               std::unique_ptr<Configuration> configuration,
               std::unique_ptr<Models> models,
               std::unique_ptr<Threads> threads,
               std::unique_ptr<Turns> turns,
               std::unique_ptr<Events> events,
               std::unique_ptr<Requests> requests);

        class Impl;
        std::unique_ptr<Impl> impl;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_CLIENT_H
