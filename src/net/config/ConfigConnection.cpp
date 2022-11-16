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

#include "net/config/ConfigConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

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

    ConfigConnection::ConfigConnection() {
        if (!getName().empty()) {
            connectionSc = add_subcommand("connection", "Options for established connections");
            connectionSc->group("Subcommands");

            readTimeoutOpt = connectionSc->add_option("--read-timeout", readTimeout, "Read timeout");
            readTimeoutOpt->type_name("[sec]");
            readTimeoutOpt->default_val(DEFAULT_READTIMEOUT);

            writeTimeoutOpt = connectionSc->add_option("--write-timeout", writeTimeout, "Write timeout");
            writeTimeoutOpt->type_name("[sec]");
            writeTimeoutOpt->default_val(DEFAULT_WRITETIMEOUT);

            readBlockSizeOpt = connectionSc->add_option("--read-block-size", readBlockSize, "Read block size");
            readBlockSizeOpt->type_name("[bytes]");
            readBlockSizeOpt->default_val(DEFAULT_READBLOCKSIZE);

            writeBlockSizeOpt = connectionSc->add_option("--write-block-size", writeBlockSize, "Write block size");
            writeBlockSizeOpt->type_name("[bytes]");
            writeBlockSizeOpt->default_val(DEFAULT_WRITEBLOCKSIZE);

            terminateTimeoutOpt = connectionSc->add_option("--terminate-timeout", terminateTimeout, "Terminate timeout");
            terminateTimeoutOpt->type_name("[sec]");
            terminateTimeoutOpt->default_val(DEFAULT_TERMINATETIMEOUT);
        } else {
            readTimeout = DEFAULT_READTIMEOUT;
            writeTimeout = DEFAULT_WRITETIMEOUT;
            readBlockSize = DEFAULT_READBLOCKSIZE;
            writeBlockSize = DEFAULT_WRITEBLOCKSIZE;
            terminateTimeout = DEFAULT_TERMINATETIMEOUT;
        }
    }

    utils::Timeval ConfigConnection::getReadTimeout() const {
        return readTimeout;
    }

    void ConfigConnection::setReadTimeout(const utils::Timeval& newReadTimeoutSet) {
        readTimeout = newReadTimeoutSet;
    }

    utils::Timeval ConfigConnection::getWriteTimeout() const {
        return writeTimeout;
    }

    void ConfigConnection::setWriteTimeout(const utils::Timeval& newWriteTimeoutSet) {
        writeTimeout = newWriteTimeoutSet;
    }

    std::size_t ConfigConnection::getReadBlockSize() const {
        return readBlockSize;
    }

    void ConfigConnection::setReadBlockSize(std::size_t newReadBlockSizeSet) {
        readBlockSize = newReadBlockSizeSet;
    }

    std::size_t ConfigConnection::getWriteBlockSize() const {
        return writeBlockSize;
    }

    void ConfigConnection::setWriteBlockSize(std::size_t newWriteBlockSizeSet) {
        writeBlockSize = newWriteBlockSizeSet;
    }

    utils::Timeval ConfigConnection::getTerminateTimeout() const {
        return terminateTimeout;
    }

    void ConfigConnection::setTerminateTimeout(const utils::Timeval& newTerminateTimeoutSet) {
        terminateTimeout = newTerminateTimeoutSet;
    }

} // namespace net::config
