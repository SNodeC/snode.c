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

#ifndef NET_CONFIGLOCAL_H
#define NET_CONFIGLOCAL_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Config.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net {

    template <typename SocketAddressT>
    class ConfigLocal {
    protected:
        ConfigLocal() = default;
        virtual ~ConfigLocal() = default;

        using SocketAddress = SocketAddressT;

    public:
        const SocketAddress& getLocalAddress() {
            if (!initialized) {
                utils::Config::instance().parse(true); // Try command line parsing in case Address is not initialized using setLocalAddress

                localAddress = getAddress();
                initialized = true;
            } else if (!updated) {
                updateFromCommandLine();
                updated = true;
            }

            return localAddress;
        }

        void setLocalAddress(const SocketAddress& localAddress) {
            this->localAddress = localAddress;
            this->initialized = true;
        }

    private:
        virtual void updateFromCommandLine() = 0;

        virtual SocketAddress getAddress() const = 0;
        virtual bool isPresent() const = 0;

    protected:
        SocketAddress localAddress;

    private:
        bool initialized = false;
        bool updated = false;
    };

} // namespace net

#endif // NET_CONFIGLOCAL_H
