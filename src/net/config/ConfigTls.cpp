/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "net/config/ConfigTls.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigTls::ConfigTls() {
        tlsSc = add_subcommand("tls");
        tlsSc->description("Options for SSL/TLS behaviour");

        initTimeoutOpt = tlsSc->add_option("-i,--init-timeout", initTimeout, "SSL/TLS initialization timeout");
        initTimeoutOpt->type_name("[sec]");
        initTimeoutOpt->default_val(10);

        shutdownTimeoutOpt = tlsSc->add_option("-s,--shutdown-timeout", shutdownTimeout, "SSL/TLS shutdown timeout");
        shutdownTimeoutOpt->type_name("[sec]");
        shutdownTimeoutOpt->default_val(2);
    }

    utils::Timeval ConfigTls::getInitTimeout() const {
        utils::Timeval initTimeout = this->initTimeout;

        if (initTimeout >= 0 && initTimeoutOpt->count() == 0) {
            initTimeout = initTimeoutSet;
        }

        return initTimeout;
    }

    utils::Timeval ConfigTls::getShutdownTimeout() const {
        utils::Timeval shutdownTimeout = this->shutdownTimeout;

        if (shutdownTimeout >= 0 && shutdownTimeoutOpt->count() == 0) {
            shutdownTimeout = shutdownTimeoutSet;
        }

        return shutdownTimeout;
    }

    void ConfigTls::setInitTimeoutSet(const utils::Timeval& newInitTimeoutSet) {
        initTimeoutSet = newInitTimeoutSet;
    }

    void ConfigTls::setShutdownTimeoutSet(const utils::Timeval& newShutdownTimeoutSet) {
        shutdownTimeoutSet = newShutdownTimeoutSet;
    }

} // namespace net::config
