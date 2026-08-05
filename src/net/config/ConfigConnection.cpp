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

#include <algorithm>
#include <functional>
#include <limits>

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

        maximumWriteQueueBytesOpt = addOption( //
            "--maximum-write-queue-bytes",
            "Maximum bytes queued for writing (0 means unlimited)",
            "bytes",
            MAXIMUM_WRITE_QUEUE_BYTES,
            CLI::NonNegativeNumber);

        writeQueueHighWatermarkOpt = addOption( //
            "--write-queue-high-watermark",
            "Suspend a pipe source at this queue size (0 selects the legacy automatic threshold)",
            "bytes",
            WRITE_QUEUE_HIGH_WATERMARK,
            CLI::NonNegativeNumber);

        writeQueueLowWatermarkOpt = addOption( //
            "--write-queue-low-watermark",
            "Resume a suspended pipe source at this queue size",
            "bytes",
            WRITE_QUEUE_LOW_WATERMARK,
            CLI::NonNegativeNumber);

        terminateTimeoutOpt = addOption( //
            "--terminate-timeout",
            "Terminate timeout",
            "timeout",
            TERMINATE_TIMEOUT,
            CLI::PositiveNumber);

        finalCallback([this]() {
            const std::size_t maximum = getMaximumWriteQueueBytes();
            const std::size_t high = getWriteQueueHighWatermark();
            const std::size_t low = getWriteQueueLowWatermark();

            const std::size_t blockSize = getWriteBlockSize();
            const std::size_t legacyHigh =
                blockSize > (std::numeric_limits<std::size_t>::max() - 1) / 5 ? std::numeric_limits<std::size_t>::max() : blockSize * 5 + 1;
            const std::size_t effectiveHigh = high != 0 ? high : (maximum != 0 ? std::min(legacyHigh, maximum) : legacyHigh);

            if (high != 0 && maximum != 0 && high > maximum) {
                throw CLI::ValidationError("--write-queue-high-watermark", "must not exceed --maximum-write-queue-bytes");
            }
            if (low > effectiveHigh) {
                throw CLI::ValidationError("--write-queue-low-watermark", "must not exceed --write-queue-high-watermark");
            }
            if (maximum != 0 && low > maximum) {
                throw CLI::ValidationError("--write-queue-low-watermark", "must not exceed --maximum-write-queue-bytes");
            }
        });
    }

    ConfigConnection::~ConfigConnection() {
    }

    utils::Timeval ConfigConnection::getReadTimeout() const {
        return readTimeoutOpt->as<utils::Timeval>();
    }

    ConfigConnection* ConfigConnection::setReadTimeout(const utils::Timeval& newReadTimeoutSet) {
        setDefaultValue(readTimeoutOpt, newReadTimeoutSet);

        return this;
    }

    utils::Timeval ConfigConnection::getWriteTimeout() const {
        return writeTimeoutOpt->as<utils::Timeval>();
    }

    ConfigConnection* ConfigConnection::setWriteTimeout(const utils::Timeval& newWriteTimeoutSet) {
        setDefaultValue(writeTimeoutOpt, newWriteTimeoutSet);

        return this;
    }

    std::size_t ConfigConnection::getReadBlockSize() const {
        return readBlockSizeOpt->as<std::size_t>();
    }

    ConfigConnection* ConfigConnection::setReadBlockSize(std::size_t newReadBlockSize) {
        setDefaultValue(readBlockSizeOpt, newReadBlockSize);

        return this;
    }

    std::size_t ConfigConnection::getWriteBlockSize() const {
        return writeBlockSizeOpt->as<std::size_t>();
    }

    ConfigConnection* ConfigConnection::setWriteBlockSize(std::size_t newWriteBlockSize) {
        setDefaultValue(writeBlockSizeOpt, newWriteBlockSize);

        return this;
    }

    std::size_t ConfigConnection::getMaximumWriteQueueBytes() const {
        return maximumWriteQueueBytesOpt->as<std::size_t>();
    }

    ConfigConnection* ConfigConnection::setMaximumWriteQueueBytes(std::size_t maximumWriteQueueBytes) {
        setDefaultValue(maximumWriteQueueBytesOpt, maximumWriteQueueBytes);

        return this;
    }

    std::size_t ConfigConnection::getWriteQueueHighWatermark() const {
        return writeQueueHighWatermarkOpt->as<std::size_t>();
    }

    ConfigConnection* ConfigConnection::setWriteQueueHighWatermark(std::size_t writeQueueHighWatermark) {
        setDefaultValue(writeQueueHighWatermarkOpt, writeQueueHighWatermark);

        return this;
    }

    std::size_t ConfigConnection::getWriteQueueLowWatermark() const {
        return writeQueueLowWatermarkOpt->as<std::size_t>();
    }

    ConfigConnection* ConfigConnection::setWriteQueueLowWatermark(std::size_t writeQueueLowWatermark) {
        setDefaultValue(writeQueueLowWatermarkOpt, writeQueueLowWatermark);

        return this;
    }

    utils::Timeval ConfigConnection::getTerminateTimeout() const {
        return terminateTimeoutOpt->as<utils::Timeval>();
    }

    ConfigConnection* ConfigConnection::setTerminateTimeout(const utils::Timeval& newTerminateTimeout) {
        setDefaultValue(terminateTimeoutOpt, newTerminateTimeout);

        return this;
    }

} // namespace net::config
