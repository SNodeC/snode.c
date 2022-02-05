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

#ifndef CORE_INITCONNECTEVENTRECEIVER_H
#define CORE_INITCONNECTEVENTRECEIVER_H

#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_WRITE_INACTIVITY 60

namespace core {

    class InitConnectEventReceiver : public DescriptorEventReceiver {
    protected:
        InitConnectEventReceiver();

    private:
        virtual void initConnectEvent() = 0;

        void dispatchEvent() final;
        void timeoutEvent() final;
        bool continueImmediately() const final;
    };

} // namespace core

#endif // CORE_INITCONNECTEVENTRECEIVER_H
