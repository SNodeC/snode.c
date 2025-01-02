/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef CORE_EVENTRECEIVER_EXCEPTIONALCONDITIONEVENTRECEIVER_H
#define CORE_EVENTRECEIVER_EXCEPTIONALCONDITIONEVENTRECEIVER_H

#include "core/DescriptorEventReceiver.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_OUTOFBAND_INACTIVITY 60

namespace core::eventreceiver {

    class ExceptionalConditionEventReceiver : public core::DescriptorEventReceiver {
    protected:
        ExceptionalConditionEventReceiver(const std::string& name, const utils::Timeval& timeout = MAX_OUTOFBAND_INACTIVITY);

        virtual void outOfBandTimeout();

    private:
        virtual void outOfBandEvent() = 0;

        void dispatchEvent() final;
        void timeoutEvent() final;
        void signalEvent(int signum) override;
    };

} // namespace core::eventreceiver

#endif // CORE_EVENTRECEIVER_EXCEPTIONALCONDITIONEVENTRECEIVER_H
