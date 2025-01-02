/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigConnection::ConfigConnection(ConfigInstance* instance)
        : net::config::ConfigSection(instance, "connection", "Configuration of established connections") {
        readTimeoutOpt = addOption( //
            "--read-timeout",
            "Read timeout in seconds",
            "timeout",
            READ_TIMEOUT,
            CLI::PositiveNumber);

        writeTimeoutOpt = addOption( //
            "--write-timeout",
            "Write timeout in seconds",
            "timeout",
            WRITE_TIMEOUT,
            CLI::PositiveNumber);

        readBlockSizeOpt = addOption( //
            "--read-block-size",
            "Read block size",
            "size",
            READ_BLOCKSIZE,
            CLI::PositiveNumber);

        writeBlockSizeOpt = addOption( //
            "--write-block-size",
            "Write block size",
            "size",
            WRITE_BLOCKSIZE,
            CLI::PositiveNumber);

        terminateTimeoutOpt = addOption( //
            "--terminate-timeout",
            "Terminate timeout",
            "timeout",
            TERMINATE_TIMEOUT,
            CLI::PositiveNumber);
    }

    utils::Timeval ConfigConnection::getReadTimeout() const {
        return readTimeoutOpt->as<utils::Timeval>();
    }

    ConfigConnection& ConfigConnection::setReadTimeout(const utils::Timeval& newReadTimeoutSet) {
        readTimeoutOpt //
            ->default_val(newReadTimeoutSet)
            ->clear();

        return *this;
    }

    utils::Timeval ConfigConnection::getWriteTimeout() const {
        return writeTimeoutOpt->as<utils::Timeval>();
    }

    ConfigConnection& ConfigConnection::setWriteTimeout(const utils::Timeval& newWriteTimeoutSet) {
        writeTimeoutOpt //
            ->default_val(newWriteTimeoutSet)
            ->clear();

        return *this;
    }

    std::size_t ConfigConnection::getReadBlockSize() const {
        return readBlockSizeOpt->as<std::size_t>();
    }

    ConfigConnection& ConfigConnection::setReadBlockSize(std::size_t newReadBlockSize) {
        readBlockSizeOpt //
            ->default_val(newReadBlockSize)
            ->clear();

        return *this;
    }

    std::size_t ConfigConnection::getWriteBlockSize() const {
        return writeBlockSizeOpt->as<std::size_t>();
    }

    ConfigConnection& ConfigConnection::setWriteBlockSize(std::size_t newWriteBlockSize) {
        writeBlockSizeOpt //
            ->default_val(newWriteBlockSize)
            ->clear();

        return *this;
    }

    utils::Timeval ConfigConnection::getTerminateTimeout() const {
        return terminateTimeoutOpt->as<utils::Timeval>();
    }

    ConfigConnection& ConfigConnection::setTerminateTimeout(const utils::Timeval& newTerminateTimeout) {
        terminateTimeoutOpt //
            ->default_val(newTerminateTimeout)
            ->clear();

        return *this;
    }

} // namespace net::config
