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

#ifndef NET_READEVENTRECEIVER_H
#define NET_READEVENTRECEIVER_H

#include "net/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_READ_INACTIVITY 60

namespace net {

    class ReadEventReceiver : public DescriptorEventReceiver {
    protected:
        ReadEventReceiver(long timeout = MAX_READ_INACTIVITY);

    private:
        virtual void readEvent() = 0;
        virtual void readTimeout();
        virtual bool continueReadImmediately();

        void dispatchEvent() final;
        void timeoutEvent() final;
        bool continueImmediately() final;
    };

} // namespace net

#endif // NET_READEVENTRECEIVER_H
