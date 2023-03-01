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

#include "net/config/ConfigConnection.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef DEFAULT_READTIMEOUT
#define DEFAULT_READTIMEOUT 60
#endif

#ifndef DEFAULT_WRITETIMEOUT
#define DEFAULT_WRITETIMEOUT 60
#endif

#ifndef DEFAULT_READBLOCKSIZE
#define DEFAULT_READBLOCKSIZE 16384
#endif

#ifndef DEFAULT_WRITEBLOCKSIZE
#define DEFAULT_WRITEBLOCKSIZE 16384
#endif

#ifndef DEFAULT_TERMINATETIMEOUT
#define DEFAULT_TERMINATETIMEOUT 1
#endif

namespace net::config {

    ConfigConnection::ConfigConnection(ConfigInstance* instance)
        : net::config::ConfigSection(instance, "connection", "Options for established connections") {
        add_option(readTimeoutOpt, //
                   "--read-timeout",
                   "Read timeout in seconds",
                   "timeout",
                   DEFAULT_READTIMEOUT,
                   CLI::PositiveNumber);
        add_option(writeTimeoutOpt, //
                   "--write-timeout",
                   "Write timeout in seconds",
                   "timeout",
                   DEFAULT_WRITETIMEOUT,
                   CLI::PositiveNumber);
        add_option(readBlockSizeOpt, //
                   "--read-block-size",
                   "Read block size",
                   "size",
                   DEFAULT_READBLOCKSIZE,
                   CLI::PositiveNumber);
        add_option(writeBlockSizeOpt, //
                   "--write-block-size",
                   "Write block size",
                   "size",
                   DEFAULT_WRITEBLOCKSIZE,
                   CLI::PositiveNumber);
        add_option(terminateTimeoutOpt, //
                   "--terminate-timeout",
                   "Terminate timeout",
                   "timeout",
                   DEFAULT_TERMINATETIMEOUT,
                   CLI::PositiveNumber);
    }

    utils::Timeval ConfigConnection::getReadTimeout() const {
        return readTimeoutOpt->as<utils::Timeval>();
    }

    void ConfigConnection::setReadTimeout(const utils::Timeval& newReadTimeoutSet) {
        readTimeoutOpt //
            ->default_val(newReadTimeoutSet)
            ->clear();
    }

    utils::Timeval ConfigConnection::getWriteTimeout() const {
        return writeTimeoutOpt->as<utils::Timeval>();
    }

    void ConfigConnection::setWriteTimeout(const utils::Timeval& newWriteTimeoutSet) {
        writeTimeoutOpt //
            ->default_val(newWriteTimeoutSet)
            ->clear();
    }

    std::size_t ConfigConnection::getReadBlockSize() const {
        return readBlockSizeOpt->as<std::size_t>();
    }

    void ConfigConnection::setReadBlockSize(std::size_t newReadBlockSize) {
        readBlockSizeOpt //
            ->default_val(newReadBlockSize)
            ->clear();
    }

    std::size_t ConfigConnection::getWriteBlockSize() const {
        return writeBlockSizeOpt->as<std::size_t>();
    }

    void ConfigConnection::setWriteBlockSize(std::size_t newWriteBlockSize) {
        writeBlockSizeOpt //
            ->default_val(newWriteBlockSize)
            ->clear();
    }

    utils::Timeval ConfigConnection::getTerminateTimeout() const {
        return terminateTimeoutOpt->as<utils::Timeval>();
    }

    void ConfigConnection::setTerminateTimeout(const utils::Timeval& newTerminateTimeout) {
        terminateTimeoutOpt //
            ->default_val(newTerminateTimeout)
            ->clear();
    }

} // namespace net::config
