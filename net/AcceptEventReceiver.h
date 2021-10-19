/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef NET_ACCEPTEVENTRECEIVER_H
#define NET_ACCEPTEVENTRECEIVER_H

#include "net/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <climits> // for LONG_MAX

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_ACCEPT_INACTIVITY LONG_MAX

namespace net {

    class AcceptEventReceiver : public DescriptorEventReceiver {
    protected:
        AcceptEventReceiver(long timeout = MAX_ACCEPT_INACTIVITY);

    private:
        virtual void acceptEvent() = 0;
        virtual void acceptTimeout();

        void dispatchEvent() final;
        void timeoutEvent() final;
        bool continueImmediately() final;

    protected:
        virtual bool continueAcceptImmediately();
    };

} // namespace net

#endif // NET_ACCEPTEVENTRECEIVER_H
