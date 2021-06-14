/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "EchoInterface.h"

#include "Echo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::subprotocol::echo::server {

    class EchoInterface : public web::ws::server::SubProtocolInterface {
    private:
        void destroy() override {
            delete this;
        }

        ROLE role() override {
            return ROLE::SERVER;
        }

        std::string name() override {
            return NAME;
        }

        web::ws::SubProtocol* create() override {
            return new Echo();
        }

        void destroy(web::ws::SubProtocol* echo) override {
            delete echo;
        }
    };

    extern "C" {
        class web::ws::server::SubProtocolInterface* plugin() {
            return new EchoInterface();
        }
    }

} // namespace web::ws::subprotocol::echo::server
