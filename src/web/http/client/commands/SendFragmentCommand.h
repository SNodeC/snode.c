/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WEB_HTTP_CLIENT_COMMANDS_SENDFRAGMENTCOMMAND_H
#define WEB_HTTP_CLIENT_COMMANDS_SENDFRAGMENTCOMMAND_H

#include "web/http/client/RequestCommand.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client::commands {

    class SendFragmentCommand : public web::http::client::RequestCommand {
    public:
        SendFragmentCommand(const char* junk, std::size_t junkLen);
        ~SendFragmentCommand() override = default;

        // RequestCommand interface
        void execute(Request* request) override;

    private:
        char* junk;
        std::size_t junkLen;
    };

} // namespace web::http::client::commands

#endif // WEB_HTTP_CLIENT_COMMANDS_SENDFRAGMENTCOMMAND_H
