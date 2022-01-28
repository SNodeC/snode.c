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

#include "net/in/stream/ServerConfig.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream {

    ServerConfig::ServerConfig(const std::string& name)
        : net::ServerConfig(name) {
        /*
                this->name = name;

                serverSc = utils::Config::instance().add_subcommand(name, "Configuration of the server");
                serverSc->configurable();

                serverBacklogOpt = serverSc->add_option("-b,--backlog,backlog", backlog, "Server listen backlog");
                serverBacklogOpt->type_name("[backlog]");
                serverBacklogOpt->default_val(5);
                serverBacklogOpt->configurable();
        */

        serverBindSc = serverSc->add_subcommand("bind");
        serverBindSc->description("Server socket bind options");
        serverBindSc->configurable();

        bindServerHostOpt = serverBindSc->add_option("-a,--host,host", bindInterface, "Bind host name or IP address");
        bindServerHostOpt->type_name("[hostname|ip]");
        bindServerHostOpt->default_val("0.0.0.0");
        bindServerHostOpt->configurable();

        bindServerPortOpt = serverBindSc->add_option("-p,--port,port", bindPort, "Bind port number");
        bindServerPortOpt->type_name("[port number]");
        bindServerPortOpt->default_val(0);
        bindServerPortOpt->configurable();

        /*
                serverConnectionSc = serverSc->add_subcommand("connection");
                serverConnectionSc->description("Options for established client connections");
                serverConnectionSc->configurable();

                serverConnectionReadTimeoutOpt = serverConnectionSc->add_option("-r,--readtimeout,readtimeout", readTimeout, "Read
           timeout"); serverConnectionReadTimeoutOpt->type_name("[sec]"); serverConnectionReadTimeoutOpt->default_val(60);
                serverConnectionReadTimeoutOpt->configurable();

                serverConnectionWriteTimeoutOpt = serverConnectionSc->add_option("-w,--writetimeout,writetimeout", writeTimeout, "Write
           timeout"); serverConnectionWriteTimeoutOpt->type_name("[sec]"); serverConnectionWriteTimeoutOpt->default_val(60);
                serverConnectionWriteTimeoutOpt->configurable();
        */
    }

    const std::string& ServerConfig::getBindInterface() const {
        return bindInterface;
    }

    uint16_t ServerConfig::getBindPort() const {
        return bindPort;
    }
    /*
        int ServerConfig::getBacklog() const {
            return backlog;
        }

        int ServerConfig::getReadTimeout() const {
            return readTimeout;
        }

        int ServerConfig::getWriteTimeout() const {
            return writeTimeout;
        }
    */
    int ServerConfig::parse(bool required) {
        utils::Config::instance().required(serverSc, required);
        utils::Config::instance().required(serverBindSc, required);
        utils::Config::instance().required(bindServerPortOpt, required);

        try {
            utils::Config::instance().parse();
        } catch (const CLI::ParseError& e) {
        }

        return 0;
    }

} // namespace net::in::stream
