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

#include "net/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    Config::Config(const std::string& name)
        : name(name) {
        serverSc = utils::Config::instance().add_subcommand(name, "Server configuration");
        serverSc->configurable();
    }

    void Config::finish() {
        connectionSc = serverSc->add_subcommand("conn");
        connectionSc->description("Options for established client connections");
        connectionSc->configurable();

        connectionSc->group("Sub-Options (use -h,--help on them)");

        readTimeoutOpt = connectionSc->add_option("-r,--read-timeout", readTimeout, "Read timeout");
        readTimeoutOpt->type_name("[sec]");
        readTimeoutOpt->default_val(60);
        readTimeoutOpt->configurable();

        writeTimeoutOpt = connectionSc->add_option("-w,--write-timeout", writeTimeout, "Write timeout");
        writeTimeoutOpt->type_name("[sec]");
        writeTimeoutOpt->default_val(60);
        writeTimeoutOpt->configurable();

        readBlockSizeOpt = connectionSc->add_option("--read-block-size", readBlockSize, "Read block size");
        readBlockSizeOpt->type_name("[bytes]");
        readBlockSizeOpt->default_val(4096);
        readBlockSizeOpt->configurable();

        writeBlockSizeOpt = connectionSc->add_option("--write-block-size", writeBlockSize, "Write block size");
        writeBlockSizeOpt->type_name("[bytes]");
        writeBlockSizeOpt->default_val(4096);
        writeBlockSizeOpt->configurable();
    }

    const std::string& Config::getName() const {
        return name;
    }

    const utils::Timeval& Config::getReadTimeout() const {
        return readTimeout;
    }

    const utils::Timeval& Config::getWriteTimeout() const {
        return writeTimeout;
    }

    std::size_t Config::getReadBlockSize() const {
        return readBlockSize;
    }

    std::size_t Config::getWriteBlockSize() const {
        return writeBlockSize;
    }

} // namespace net
