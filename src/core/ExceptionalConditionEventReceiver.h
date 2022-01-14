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

#ifndef CORE_EXCEPTIONALCONDITIONEVENTRECEIVER_H
#define CORE_EXCEPTIONALCONDITIONEVENTRECEIVER_H

#include "core/EventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_OUTOFBAND_INACTIVITY 60

namespace core {

    class ExceptionalConditionEventReceiver : public EventReceiver {
    protected:
        ExceptionalConditionEventReceiver(const utils::Timeval& timeout = MAX_OUTOFBAND_INACTIVITY);

    private:
        virtual void outOfBandEvent() = 0;
        virtual void outOfBandTimeout();

        void dispatchEvent() final;
        void timeoutEvent() final;
        bool continueImmediately() const final;

        virtual bool continueOutOfBandImmediately() const;
    };

} // namespace core

#endif // CORE_EXCEPTIONALCONDITIONEVENTRECEIVER_H
