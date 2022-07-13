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

#ifndef NET_CONFIG_CONFIGCONN_H
#define NET_CONFIG_CONFIGCONN_H

#include "net/config/ConfigBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include "utils/Timeval.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigConnection : virtual public ConfigBase {
    public:
        ConfigConnection();

    public:
        utils::Timeval getReadTimeout() const;
        utils::Timeval getWriteTimeout() const;

        std::size_t getReadBlockSize() const;
        std::size_t getWriteBlockSize() const;

        utils::Timeval getTerminateTimeout() const;

        void setReadTimeout(const utils::Timeval& newReadTimeoutSet);
        void setWriteTimeout(const utils::Timeval& newWriteTimeoutSet);

        void setReadBlockSize(std::size_t newReadBlockSizeSet);
        void setWriteBlockSize(std::size_t newWriteBlockSizeSet);

        void setTerminateTimeout(const utils::Timeval& newTerminateTimeoutSet);

    private:
        CLI::App* connectionSc = nullptr;
        CLI::Option* readTimeoutOpt = nullptr;
        CLI::Option* writeTimeoutOpt = nullptr;

        CLI::Option* readBlockSizeOpt = nullptr;
        CLI::Option* writeBlockSizeOpt = nullptr;

        CLI::Option* terminateTimeoutOpt = nullptr;

        utils::Timeval readTimeout;
        utils::Timeval readTimeoutSet = -1;

        utils::Timeval writeTimeout;
        utils::Timeval writeTimeoutSet = -1;

        std::size_t readBlockSize;
        std::size_t readBlockSizeSet = 0;

        std::size_t writeBlockSize;
        std::size_t writeBlockSizeSet = 0;

        utils::Timeval terminateTimeout;
        utils::Timeval terminateTimeoutSet = -1;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGCONN_H
