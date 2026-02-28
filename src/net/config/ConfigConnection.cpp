/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "net/config/ConfigConnection.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigConnection::ConfigConnection(ConfigInstance* instance)
        : net::config::ConfigSection(instance, this) {
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

    ConfigConnection::~ConfigConnection() {
    }

    utils::Timeval ConfigConnection::getReadTimeout() const {
        return readTimeoutOpt->as<utils::Timeval>();
    }

    ConfigConnection& ConfigConnection::setReadTimeout(const utils::Timeval& newReadTimeoutSet) {
        setDefaultValue(readTimeoutOpt, newReadTimeoutSet);

        return *this;
    }

    utils::Timeval ConfigConnection::getWriteTimeout() const {
        return writeTimeoutOpt->as<utils::Timeval>();
    }

    ConfigConnection& ConfigConnection::setWriteTimeout(const utils::Timeval& newWriteTimeoutSet) {
        setDefaultValue(writeTimeoutOpt, newWriteTimeoutSet);

        return *this;
    }

    std::size_t ConfigConnection::getReadBlockSize() const {
        return readBlockSizeOpt->as<std::size_t>();
    }

    ConfigConnection& ConfigConnection::setReadBlockSize(std::size_t newReadBlockSize) {
        setDefaultValue(readBlockSizeOpt, newReadBlockSize);

        return *this;
    }

    std::size_t ConfigConnection::getWriteBlockSize() const {
        return writeBlockSizeOpt->as<std::size_t>();
    }

    ConfigConnection& ConfigConnection::setWriteBlockSize(std::size_t newWriteBlockSize) {
        setDefaultValue(writeBlockSizeOpt, newWriteBlockSize);

        return *this;
    }

    utils::Timeval ConfigConnection::getTerminateTimeout() const {
        return terminateTimeoutOpt->as<utils::Timeval>();
    }

    ConfigConnection& ConfigConnection::setTerminateTimeout(const utils::Timeval& newTerminateTimeout) {
        setDefaultValue(terminateTimeoutOpt, newTerminateTimeout);

        return *this;
    }

} // namespace net::config
