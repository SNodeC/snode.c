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

#ifndef NET_CONFIGCONN_H
#define NET_CONFIGCONN_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include "utils/Timeval.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    class ConfigConn {
    public:
        explicit ConfigConn(CLI::App* baseSc);

    public:
        const utils::Timeval& getReadTimeout() const;

        const utils::Timeval& getWriteTimeout() const;

        std::size_t getReadBlockSize() const;

        std::size_t getWriteBlockSize() const;

        const utils::Timeval& getTerminateTimeout() const;

    private:
        CLI::App* connectionSc = nullptr;
        CLI::Option* readTimeoutOpt = nullptr;
        CLI::Option* writeTimeoutOpt = nullptr;
        CLI::Option* readBlockSizeOpt = nullptr;
        CLI::Option* writeBlockSizeOpt = nullptr;
        CLI::Option* terminateTimeoutOpt = nullptr;

        std::string name;

        utils::Timeval readTimeout;
        utils::Timeval writeTimeout;

        std::size_t readBlockSize;
        std::size_t writeBlockSize;

        utils::Timeval terminateTimeout;
    };

} // namespace net

#endif // CONFIGCONN_H
