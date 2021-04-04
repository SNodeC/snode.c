/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef CONNECTEVENTRECEIVER_H
#define CONNECTEVENTRECEIVER_H

#include "net/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_CONNECT_INACTIVITY 10

namespace net {

    class ConnectEventReceiver : public DescriptorEventReceiver {
    protected:
        ConnectEventReceiver(long timeout = MAX_CONNECT_INACTIVITY);

        virtual void connectEvent() = 0;

    private:
        void dispatchEvent() override;
    };

} // namespace net

#endif // CONNECTEVENTRECEIVER_H
